#include "FileUtil.h"

#include <iostream>
#include <fstream>
#include <regex>
#include <boost/filesystem.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

std::vector<std::string> FileUtil::ScanFiles(const std::string& root_path, const std::string& regex_filter)
{
	std::vector<std::string> v_files;
	namespace fs = boost::filesystem;
	fs::path fullpath(root_path, fs::native);

	if (!fs::exists(fullpath))
	{
		return  v_files;
	}

	fs::directory_iterator end_iter;
	for (fs::directory_iterator iter(fullpath); iter != end_iter; iter++)
	{
		try
		{
			std::string file_full_path = iter->path().string();
			if (fs::is_directory(file_full_path))
			{
				//auto v = FileUtil::ScanFiles(file_full_path,regex_filter);
				//v_files.insert(v_files.end(), v.begin(), v.end());
			}
			else
			{
				if (regex_filter.empty() || std::regex_search(file_full_path, std::regex(regex_filter)))
				{
					v_files.push_back(file_full_path);
				}
			}
		}
		catch (const std::exception & ex)
		{
			std::cerr << "ScanFilesDfs exception:" << ex.what() << std::endl;
			continue;
		}
	}
	return v_files;
}

std::vector<std::string> FileUtil::ScanFilesRecursive(const std::string& root_path, const std::string& regex_filter)
{
	std::vector<std::string> v_files;

	namespace fs = boost::filesystem;
	fs::path fullpath(root_path, fs::native);

	if (!fs::exists(fullpath))
	{
		return v_files;
	}

	fs::recursive_directory_iterator end_iter;
	for (fs::recursive_directory_iterator iter(fullpath); iter != end_iter; iter++)
	{
		try
		{
			std::string file_full_path = iter->path().string();
			if (!fs::is_directory(file_full_path))
			{
				if (regex_filter.empty() || std::regex_search(file_full_path, std::regex(regex_filter)))
				{
					v_files.push_back(file_full_path);
				}
			}
		}
		catch (const std::exception & ex)
		{
			std::cerr << "ScanFilesDfs exception:" << ex.what() << std::endl;
			continue;
		}
	}
	return v_files;
}

std::string FileUtil::ReadFile(const std::string& file_full_name, bool binary_mode)
{
	uint8_t open_mode = std::ios::in;
	if (binary_mode) open_mode |= std::ios::binary;

	std::ifstream ifs(file_full_name.c_str(), open_mode);
	std::string content((std::istreambuf_iterator<char>(ifs)),
		(std::istreambuf_iterator<char>()));
	ifs.close();
	return content;
}

bool FileUtil::ReadFile(const std::string& file_full_name, std::vector<std::string>& v_line)
{
	std::fstream fs;
	fs.open(file_full_name.c_str(), std::ios::in | std::ios::out);
	if (fs.is_open())
	{
		std::string strLine;

		while (std::getline(fs, strLine, '\n'))
		{
			//如果最后一个字符是回车，去掉。
			if (!strLine.empty())
			{
				auto it = strLine.end() - 1;
				if (*it == '\r')
				{
					strLine.erase(it);
				}
			}
			v_line.push_back(strLine);
		}
		if (fs.eof())
			return true;
	}
	printf("ReadFile %s failed,%d,%s", file_full_name.c_str(), errno, strerror(errno));
	return false;
}


bool FileUtil::WriteFile(const std::string& file_full_name, const std::string& content, bool append, bool binary_mode)
{
	boost::filesystem::path p(file_full_name);
	if (!p.parent_path().string().empty() && !boost::filesystem::exists(p))
	{
		boost::system::error_code ec;
		boost::filesystem::create_directories(p.parent_path(), ec);

		if (ec)
		{
			printf("create_directories failed,path:%s,%d,%s", p.parent_path().string().c_str(), ec.value(), ec.message().c_str());
			return false;
		}
	}

	uint8_t open_mode = std::ios::out | std::ios::binary;
	append ? open_mode |= std::ios::app : open_mode |= std::ios::trunc;
	if (binary_mode) open_mode |= std::ios::binary;

	std::ofstream ofs(file_full_name.c_str(), open_mode);
	if (!ofs.is_open())
	{
		printf("open file %s failed,%d,%s", file_full_name.c_str(), errno, strerror(errno));
		return false;
	}

	ofs << content;
	ofs.close();
	return true;
}

bool FileUtil::WriteFile(const std::string& file_full_name, const std::vector<std::string>& v_line, bool append, bool write_crlf)
{
	boost::filesystem::path p(file_full_name);
	if (!p.parent_path().string().empty() && !boost::filesystem::exists(p))
	{
		boost::system::error_code ec;
		boost::filesystem::create_directories(p.parent_path(), ec);

		if (ec)
		{
			printf("create_directories failed,path:%s,%d,%s", p.parent_path().string().c_str(), ec.value(), ec.message().c_str());
			return false;
		}
	}

	uint8_t open_mode = std::ios::in | std::ios::out;
	append ? open_mode |= std::ios::app : open_mode |= std::ios::trunc;
	//if (binary_mode) open_mode |= std::ios::binary;

	std::fstream fs;
	fs.open(file_full_name.c_str(), open_mode);

	if (!fs.is_open())
	{
		printf("open file %s failed,%d,%s", file_full_name.c_str(), errno, strerror(errno));
		return false;
	}
	/*
	OS	    换行符	缩写	ASCII码
	windows	\r\n	CRLF	0D0A
	linux	\n	    LF	    0A
	mac	    \r	    CR	    0D
	*/


	for (auto& line : v_line)
	{
		//写入一行加入回车换行
		fs.write(line.c_str(), line.size());
		if (write_crlf)
		{
#ifdef _WIN32
			fs << "\r\n";
#elif __linux__ 
			fs << "\n";
#endif
		}

		if (fs.fail())
			return false;
		}

	return true;
	}
