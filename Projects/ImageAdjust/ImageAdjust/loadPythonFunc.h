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
	PyObject * pModule = NULL;//��������
	PyObject * pFunc = NULL;// ��������
	PyObject * pClass = NULL;//��������
	PyObject * pInstance = NULL;
	
	string m_module_name;
	string m_class_name;
	string m_method_name;




};
