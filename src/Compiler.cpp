// Jopnal Resource C++ Application
// Copyright(c) 2016 Team Jopnal
// 
// This software is provided 'as-is', without any express or implied
// warranty.In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions :
// 
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software.If you use this software
// in a product, an acknowledgement in the product documentation would be
// appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

// Headers
#include <Compiler.hpp>

#pragma warning(push)
#pragma warning(disable: 4244)
#include <rapidjson/document.h>
#pragma warning(pop)

#include <fstream>
#include <streambuf>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

//////////////////////////////////////////////


namespace
{
    void compileSingle(const char* name, const unsigned char* data, const std::size_t dataSize, std::string& header, std::string& source)
    {
        header += "extern const unsigned char ";
        header += name;
        header += "[" + std::to_string(dataSize) + "];";

        source += "const unsigned char ";
        source += name;
        source += "[" + std::to_string(dataSize) + "] =\n{";

        std::ostringstream arrayStream;
        for (std::size_t i = 0; i < dataSize; ++i)
            arrayStream << (i % 50 == 0 ? "\n" : "") << static_cast<unsigned int>(data[i]) << ",";

        source += arrayStream.str();

        header += "\n\n";
        source += "\n};\n\n";
    }

    void insertHeader(const rapidjson::Value& root, std::string& header, const bool headerFile)
    {
        std::string fileName = (root.HasMember("filename") && root["filename"].IsString() ? root["filename"].GetString() : "jopres");

        header += "// " + fileName + (headerFile ? ".hpp" : ".cpp") + "\n// Generated with Jopnal Resource\n\n";

        if (!headerFile)
            header += "#include \"" + fileName + ".hpp\"\n\n";
        else
        {
            std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::toupper);

            std::string incGuard = "_JOPRESOURCE_";
            incGuard += fileName + "_HPP\n";

            header += "#ifndef " + incGuard;
            header += "#define " + incGuard;
        }

        header += "\nnamespace ";

        if (root.HasMember("namespace") && root["namespace"].IsString())
            header += root["namespace"].GetString();
        else
            header += "jopres";

        header += "\n{\n";
    }
}

namespace jopr
{
    int Compiler::compile(const char* descPath)
    {
        std::ifstream stream(descPath);

        if (!stream.is_open())
        {
            std::cerr << "Failed to open descriptor file" << std::endl;
            return EXIT_FAILURE;
        }

        std::string jsonStr((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

        namespace rj = rapidjson;

        rj::Document doc;
        doc.ParseInsitu<rj::kParseStopWhenDoneFlag | rj::kParseInsituFlag>(&jsonStr[0]);

        if (doc.HasParseError())
        {
            std::cerr << "Parse error (code " << doc.GetParseError() << ") at " << doc.GetErrorOffset() << std::endl;
            return EXIT_FAILURE;
        }

        if (!doc.HasMember("resources") || !doc["resources"].IsArray() || doc["resources"].Empty())
        {
            std::cerr << "Descriptor has no resource array" << std::endl;
            return EXIT_FAILURE;
        }

        std::string header, source;

        insertHeader(doc, header, true);
        insertHeader(doc, source, false);

        const rj::Value& resArray = doc["resources"];
        for (auto itr = resArray.Begin(); itr != resArray.End(); ++itr)
        {
            if (!itr->IsObject())
            {
                std::cerr << "Invalid resource definition (must be object)" << std::endl;
                continue;
            }

            if (!itr->HasMember("name") || !(*itr)["name"].IsString())
            {
                std::cerr << "Resource definition has no name" << std::endl;
                continue;
            }

            if (!itr->HasMember("path") || !(*itr)["path"].IsString())
            {
                std::cerr << "Resource \"" << (*itr)["name"].GetString() << "\" has no file path" << std::endl;
                continue;
            }

            std::ifstream resStream((*itr)["path"].GetString(), std::ifstream::binary);

            if (!resStream.is_open())
            {
                std::cerr << "Failed to open resource \"" << (*itr)["name"].GetString() << "\" for reading" << std::endl;
                continue;
            }
            
            const std::vector<char> buffer((std::istreambuf_iterator<char>(resStream)), std::istreambuf_iterator<char>());
            compileSingle((*itr)["name"].GetString(), reinterpret_cast<const unsigned char*>(buffer.data()), buffer.size(), header, source);
            
            std::cout << "Successfully compiled resource \"" << (*itr)["name"].GetString() << "\"" << std::endl;
        }

        // Close namespaces & include guard
        header += "}\n#endif";
        source += "}";

        // Write header & source
        const std::string fileName = (doc.HasMember("filename") && doc["filename"].IsString() ? doc["filename"].GetString() : "jopres");

        for (std::size_t i = 0; i < 2; ++i)
        {
            const std::string fileNameExt = fileName + (i < 1 ? ".hpp" : ".cpp");
            std::ofstream writeStream(fileNameExt);

            if (!writeStream.is_open())
            {
                std::cerr << "Failed to open \"" << fileNameExt << "\" for writing" << std::endl;
                return EXIT_FAILURE;
            }

            const std::string& str = i < 1 ? header : source;

            writeStream << str;
            writeStream.close();
        }

        return EXIT_SUCCESS;
    }
}