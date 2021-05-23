#include <ntddk.h>
#include <usbioctl.h>

#define DEVICE_NAME L"\\Driver\\Usbreset"
#define SYM_NAME L"\\??\\Usbreset"

#define IOCTL_RESET CTL_CODE(FILE_DEVICE_UNKNOWN,0x9888,METHOD_BUFFERED,FILE_ANY_ACCESS)

VOID Unload(PDRIVER_OBJECT DriverObject) {

	UNREFERENCED_PARAMETER(DriverObject);
	KdPrint(("ж����������\n"));

	if (DriverObject->DeviceObject)
	{
		IoDeleteDevice(DriverObject->DeviceObject);

		UNICODE_STRING symname = { 0 };

		RtlInitUnicodeString(&symname, SYM_NAME);

		IoDeleteSymbolicLink(&symname);

		KdPrint(("�������豸�Ƴ��ɹ�"));

	}
}

//
//Dispatch
//

NTSTATUS MyCreate(PDEVICE_OBJECT pdevice, PIRP pirp)

{
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("My device has bee opened\n");

	pirp->IoStatus.Status = status;
	pirp->IoStatus.Information = 0;
	IoCompleteRequest(pirp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS MyClose(PDEVICE_OBJECT pdevice, PIRP pirp)

{
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("My device has been closed\n");

	pirp->IoStatus.Status = status;
	pirp->IoStatus.Information = 0;
	IoCompleteRequest(pirp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS MyCleanUp(PDEVICE_OBJECT pdevice, PIRP pirp)

{
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("My device has been clanup\n");

	pirp->IoStatus.Status = status;
	pirp->IoStatus.Information = 0;
	IoCompleteRequest(pirp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS HubResetControl(PDEVICE_OBJECT pdevice, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	//��ʼ��USB hub������reset
	KdPrint(("Enter HubResetControl\n"));

	//�õ���ǰ��ջ
	PIO_STACK_LOCATION pstack = IoGetCurrentIrpStackLocation(pIrp);
	//�õ�IOCTL��
	ULONG  iocode = pstack->Parameters.DeviceIoControl.IoControlCode;
	//�õ����뻺������С
	ULONG inlen = pstack->Parameters.DeviceIoControl.InputBufferLength;
	//�õ������������С
	ULONG outlen = pstack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (iocode)
	{
	case IOCTL_RESET:
	{

		//����豸����IoGetDeviceObjectPointer
		UNICODE_STRING DeviceName;

		PDEVICE_OBJECT DeviceObject = NULL;

		PFILE_OBJECT FileObject = NULL;

		RtlInitUnicodeString(&DeviceName, L"\\Device\\USBPDO-1");

		status = IoGetDeviceObjectPointer(&DeviceName, FILE_ALL_ACCESS, &FileObject, &DeviceObject);

		DbgPrint("HubDriver:FileObject:%x\n", FileObject);

		DbgPrint("HubDriver:DeviceObject:0x%p\n", DeviceObject);

		//����������irp����

		KEVENT Event;
		IO_STATUS_BLOCK iostatus;
		PIRP Irp;
		PIO_STACK_LOCATION stack;

		KeInitializeEvent(&Event, NotificationEvent, FALSE);
		Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_RESET_PORT, DeviceObject, NULL, 0, NULL, 0, TRUE, &Event, &iostatus);
		KdPrint(("FDO_CycleUsbPort: IoBuildDeviceIoControlRequest, Irp %X \n", Irp));
		//ȡ���豸��������IO����
		IoCancelIrp(Irp);

		Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_RESET_PORT, DeviceObject, NULL, 0, NULL, 0, TRUE, &Event, &iostatus);
		KdPrint(("FDO_CycleUsbPort: IoBuildDeviceIoControlRequest, Irp %X \n", Irp));

		stack = IoGetNextIrpStackLocation(Irp);
		stack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
		stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_RESET_PORT;

		KdPrint(("FDO_CycleUsbPort: IoCallDriver(IOCTL_INTERNAL_USB_CYCLE_PORT, DeviceObject 0x%p)\n", DeviceObject));
		status = IoCallDriver(DeviceObject, Irp);

		if (status == STATUS_PENDING)
		{
			KdPrint(("FDO_CycleUsbPort: wait IoCallDriver complete ...\n"));
			KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
			status = iostatus.Status;
		}

		KdPrint(("FDO_CycleUsbPort: IoCallDriver return status 0x%x, iostatus.Status 0x%x\n", status, iostatus.Status));
		if (!NT_SUCCESS(status))
		{
			DbgPrint("FDO_CycleUsbPort: Error %X trying to reset device\n", status);
		}

		KdPrint(("FDO_CycleUsbPort: Leave, status 0x%x\n", status));

		break;
	}
	default:

		status = STATUS_INVALID_DEVICE_REQUEST;
		
		break;
	}

	//���÷���״̬
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint((" Leave HubResetControl\n"));

	return status;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS status = STATUS_SUCCESS;

	KdPrint(("DriverEntry\n"));

	UNICODE_STRING devicename = { 0 };

	PDEVICE_OBJECT pdevice = NULL;

	DriverObject->DriverUnload = Unload;

	RtlInitUnicodeString(&devicename, DEVICE_NAME);

	status = IoCreateDevice(DriverObject, 0, &devicename, FILE_DEVICE_UNKNOWN, 0, TRUE, &pdevice);

	if (!NT_SUCCESS(status))
	{
		DbgPrint("Create device failed: %x\n", status);
		return status;
	}

	else
	{
		DbgPrint("�豸�����ɹ�\n");
	}

	//
	//�����豸�ɹ�
	//
	//��ӡע�������Ŀ¼
	KdPrint(("-----%wZ-------\n", RegistryPath));

	UNICODE_STRING symname = { 0 };

	//	UNICODE_STRING symname;

	RtlInitUnicodeString(&symname, SYM_NAME);

	status = IoCreateSymbolicLink(&symname, &devicename);

	if (!NT_SUCCESS(status))
	{
		DbgPrint("Create SymbolicLink failed: %x\n", status);

		IoDeleteDevice(pdevice);

		return status;
	}

	DbgPrint("�豸�����ɹ����豸�ѳɹ����أ�");

	DriverObject->MajorFunction[IRP_MJ_CREATE] = MyCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = MyClose;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = MyCleanUp;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HubResetControl;

	return STATUS_SUCCESS;
}