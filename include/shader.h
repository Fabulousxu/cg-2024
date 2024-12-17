#pragma once
#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);

    }
    
    Shader(const char* vertexPath, const char* fragmentPath, const char* tcs, const char* tes, const char* gs)
    {
        // 1. ���ļ�·���л�ȡ����/Ƭ����ɫ��

        const char* vShaderCode = read_shader(vertexPath);
        const char* fShaderCode = read_shader(fragmentPath);


        // 2. ������ɫ��
        unsigned int vertex, fragment;
        int success;
        char infoLog[512];

        // ������ɫ��
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        // ��ӡ�����������еĻ���
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        };

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        // ��ӡ�����������еĻ���
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        };

        // ��ɫ������
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if (tcs)
        {
            const char* cShaderCode = read_shader(tcs);
            const char* eShaderCode = read_shader(tes);


            // 2. ������ɫ��
            unsigned int tesc, tese;
            int success;
            char infoLog[512];

            // ������ɫ��
            tesc = glCreateShader(GL_TESS_CONTROL_SHADER);
            glShaderSource(tesc, 1, &cShaderCode, NULL);
            glCompileShader(tesc);
            // ��ӡ�����������еĻ���
            glGetShaderiv(tesc, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(tesc, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::tesc::COMPILATION_FAILED\n" << infoLog << std::endl;
            };

            tese = glCreateShader(GL_TESS_EVALUATION_SHADER);
            glShaderSource(tese, 1, &eShaderCode, NULL);
            glCompileShader(tese);
            // ��ӡ�����������еĻ���
            glGetShaderiv(tese, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(tese, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::tese::COMPILATION_FAILED\n" << infoLog << std::endl;
            };
            glAttachShader(ID, tese);
            glAttachShader(ID, tesc);
        }
        if (gs)
        {
            const char* gShaderCode = read_shader(gs);
        


            // 2. ������ɫ��
            unsigned int geom;
            int success;
            char infoLog[512];

            // ������ɫ��
            geom = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geom, 1, &gShaderCode, NULL);
            glCompileShader(geom);
            // ��ӡ�����������еĻ���
            glGetShaderiv(geom, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(geom, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::gs::COMPILATION_FAILED\n" << infoLog << std::endl;
            };

        
            glAttachShader(ID, geom);

        }

        glLinkProgram(ID);
        // ��ӡ���Ӵ�������еĻ���
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }

        // ɾ����ɫ���������Ѿ����ӵ����ǵĳ������ˣ��Ѿ�������Ҫ��
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    // activate the shader
    // ------------------------------------------------------------------------
    void use() const
    {
        glUseProgram(ID);
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setVec2(const std::string& name, const glm::vec2& value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const std::string& name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string& name, const glm::vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string& name, float x, float y, float z, float w) const
    {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }
    // ------------------------------------------------------------------------
    void setMat2(const std::string& name, const glm::mat2& mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat3(const std::string& name, const glm::mat3& mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }

    const char* read_shader(const char* Path)
    {
        std::string Code;
    
        std::ifstream ShaderFile;
    
        // ��֤ifstream��������׳��쳣��
       ShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
        try
        {
            // ���ļ�
            ShaderFile.open(Path);
            std::stringstream ShaderStream;
            // ��ȡ�ļ��Ļ������ݵ���������
            ShaderStream << ShaderFile.rdbuf();
        
            // �ر��ļ�������
            ShaderFile.close();
        
            // ת����������string
            Code = ShaderStream.str();

        }
        catch (std::ifstream::failure e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
            std::string* tmp = new std::string(Code);
        return tmp->c_str();
    }
};
#endif