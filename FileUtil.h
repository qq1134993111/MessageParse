#pragma once
#include<vector>
#include<string>


class FileUtil
{
public:

    static  std::vector<std::string> ScanFiles(const std::string&, const std::string& regex_filter = "");
    static  std::vector<std::string> ScanFilesRecursive(const std::string&, const std::string& regex_filter = "");


    static std::string ReadFile(const std::string& file_full_name, bool binary_mode = false);
    static bool ReadFile(const std::string& file_full_name, std::vector<std::string>& v_line);

    static bool WriteFile(const std::string& file_full_name, const std::string& content, bool append = false, bool binary_mode = false);
    static bool WriteFile(const std::string& file_full_name, const std::vector<std::string>& v_line, bool append = false, bool write_crlf = true);
};