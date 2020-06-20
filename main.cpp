// MessageParse.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <vector>
#include <tuple>
#include<unordered_map>
#include"MessageParse.h"
#include"inja/inja.hpp"


int main(int argc, char** argv)
{
	std::string  source_file;
	std::string  tmp_path;
	boost::program_options::options_description opts(" options");
	opts.add_options()
		("help,h", "help info")
		("source,s", boost::program_options::value<std::string>(&source_file)->default_value(""), "source xml file full path")
		("template_path,t", boost::program_options::value<std::string>(&tmp_path)->default_value(boost::filesystem::current_path().string()), "template file path,default current path")
		;

	boost::program_options::variables_map vm;
	try
	{
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, opts), vm); // 分析参数
	}
	catch (boost::program_options::error_with_no_option_name& ex)
	{
		std::cout << ex.what() << std::endl;
	}

	boost::program_options::notify(vm); // 将解析的结果存储到外部变量
	if (vm.count("help"))
	{
		std::cout << opts << std::endl;
		return -1;
	}

	//nlohmann::json j,j2;
	//j["array"] = { {0,std::string("0")},{1,std::string("hello")},{1,std::string("world")} };
	//j2["Obj"] = { {"str:","hello"} };
	//j["a2"] = j2;
	//std::cout << j;

	MessageParser parser;
	if (!parser.LoadXml(source_file))
	{
		std::cout << "load error\n";
		return false;
	}

	parser.Write(tmp_path,boost::filesystem::current_path().string()+"\\GenMsg");

	std::cout << "Hello World!\n";
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
