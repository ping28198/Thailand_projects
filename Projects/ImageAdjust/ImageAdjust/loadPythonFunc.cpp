#include "loadPythonFunc.h"

Yolo3_tiny::Yolo3_tiny()
{
	Py_Initialize();
}

int Yolo3_tiny::initial()
{
	if (!Py_IsInitialized())
	{
		printf("��ʼ��ʧ�ܣ�");
		return 0;
	}

	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append('./python/')");//��һ������Ҫ���޸�Python·��


	pModule = PyImport_ImportModule("runYoloForCpp");//������Ҫ���õ��ļ���hello.py
	if (pModule == NULL)
	{
		cout << "û�ҵ�Ҫִ�е�python�ļ�" << endl;
		return -1;
	}
	//PyObject* pDict = PyModule_GetDict(pModule);
	//if (!pDict) {
	//	printf("Cant find dictionary.\n");
	//	return -1;
	//}
	pClass = PyObject_GetAttrString(pModule, "YOLO");
	if (pClass == NULL) {
		printf("Cant find YOLO class.\n");
		return -1;
	}
	//pInstance = PyInstanceMethod_New(pClass, NULL);
	pInstance = PyObject_CallFunctionObjArgs(pClass,NULL);
	if (pInstance == NULL) {
		printf("Cant find YOLO instance.\n");
		return -1;
	}

	//pFunc = PyObject_GetAttrString(pModule, "YOLO");
	//if (pFunc == NULL || !PyCallable_Check(pFunc))
	//{
	//	printf("δ�ҵ�����\n");
	//	return -1;
	//}
	////��ʼ��Ҫ����Ĳ�����args���óɴ�������������ģʽ  
	//PyObject* args = PyTuple_New(2);
	////��Long������ת����Python�ɽ��յ�����  
	//PyObject* arg1 = Py_BuildValue("s","D:/Program_files/tensorflow_cpp/test/image1.jpg");
	//PyObject* arg2 = PyLong_FromLong(3);

	////��arg1����Ϊarg����ĵ�һ������  
	//PyTuple_SetItem(args, 0, arg1);
	////��arg1����Ϊarg����ĵڶ�������  
	//PyTuple_SetItem(args, 1, arg2);

	////����������ú���������ȡ����ֵ  
	//PyObject* pRet = PyObject_CallObject(pFunc, args);

	//if (pRet)
	//{
	//	//������ֵת����long��  
	//	printf("���سɹ���\n");
	//}
	//else
	//{
	//	printf("����ʧ��\n");
	//}
	//


}

int Yolo3_tiny::load_module(string module_name)
{
	if (module_name.empty())
	{
		return 0;
	}
	pModule = PyImport_ImportModule(module_name.c_str());//������Ҫ���õ��ļ���hello.py
	if (pModule == NULL)
	{
		cout << "û�ҵ�Ҫִ�е�python�ļ�" << endl;
		return -1;
	}
	else
	{

	}
}

int Yolo3_tiny::load_class(string module_name)
{
	return 1;
}

int Yolo3_tiny::detectImage(string image_path)
{
	string pPama = "D:/Program_files/tensorflow_cpp/test/image1.jpg";
	PyObject *pRet = PyObject_CallMethod(pInstance, "detect_image_file", "(s)", pPama.c_str());
	if (pRet == NULL)
	{
		printf("�����ҵ� pRet");
		return -1;
	}
	else
	{
		printf("������ȷ\n");
	}
}

Yolo3_tiny::~Yolo3_tiny()
{
	Py_Finalize();
}
