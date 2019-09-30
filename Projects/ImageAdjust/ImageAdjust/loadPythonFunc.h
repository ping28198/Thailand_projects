#pragma once
#include <Python.h>
#include <iostream>

using namespace std;

class Yolo3_tiny
{
public:
	Yolo3_tiny();
	int initial();
	int load_module(string module_name);
	int load_class(string module_name);
	int detectImage(string image_path);
	
	~Yolo3_tiny();
private:
	PyObject * pModule = NULL;//声明变量
	PyObject * pFunc = NULL;// 声明变量
	PyObject * pClass = NULL;//声明变量
	PyObject * pInstance = NULL;
	
	string m_module_name;
	string m_class_name;
	string m_method_name;




};
