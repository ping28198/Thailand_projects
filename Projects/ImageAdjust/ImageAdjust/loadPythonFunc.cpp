#include "loadPythonFunc.h"

Yolo3_tiny::Yolo3_tiny()
{
	Py_Initialize();
}

int Yolo3_tiny::initial()
{
	if (!Py_IsInitialized())
	{
		printf("初始化失败！");
		return 0;
	}

	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append('./python/')");//这一步很重要，修改Python路径


	pModule = PyImport_ImportModule("runYoloForCpp");//这里是要调用的文件名hello.py
	if (pModule == NULL)
	{
		cout << "没找到要执行的python文件" << endl;
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
	//	printf("未找到函数\n");
	//	return -1;
	//}
	////初始化要传入的参数，args配置成传入两个参数的模式  
	//PyObject* args = PyTuple_New(2);
	////将Long型数据转换成Python可接收的类型  
	//PyObject* arg1 = Py_BuildValue("s","D:/Program_files/tensorflow_cpp/test/image1.jpg");
	//PyObject* arg2 = PyLong_FromLong(3);

	////将arg1配置为arg带入的第一个参数  
	//PyTuple_SetItem(args, 0, arg1);
	////将arg1配置为arg带入的第二个参数  
	//PyTuple_SetItem(args, 1, arg2);

	////传入参数调用函数，并获取返回值  
	//PyObject* pRet = PyObject_CallObject(pFunc, args);

	//if (pRet)
	//{
	//	//将返回值转换成long型  
	//	printf("返回成功！\n");
	//}
	//else
	//{
	//	printf("调用失败\n");
	//}
	//


}

int Yolo3_tiny::load_module(string module_name)
{
	if (module_name.empty())
	{
		return 0;
	}
	pModule = PyImport_ImportModule(module_name.c_str());//这里是要调用的文件名hello.py
	if (pModule == NULL)
	{
		cout << "没找到要执行的python文件" << endl;
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
		printf("不能找到 pRet");
		return -1;
	}
	else
	{
		printf("返回正确\n");
	}
}

Yolo3_tiny::~Yolo3_tiny()
{
	Py_Finalize();
}
