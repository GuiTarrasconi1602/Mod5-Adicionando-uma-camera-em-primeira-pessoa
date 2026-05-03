#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace glm;
using namespace std;

vec3 position = vec3(0.0f);
float scaleFactor = 1.0f;

void key_control(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		if (key == GLFW_KEY_I) position.z -= 0.1f;
		if (key == GLFW_KEY_J) position.z += 0.1f;
		if (key == GLFW_KEY_W) position.y += 0.1f;
		if (key == GLFW_KEY_A) position.x -= 0.1f;
		if (key == GLFW_KEY_S) position.y -= 0.1f;
		if (key == GLFW_KEY_D) position.x += 0.1f;
		if (key == GLFW_KEY_LEFT_BRACKET) scaleFactor -= 0.1f;
		if (key == GLFW_KEY_RIGHT_BRACKET) scaleFactor += 0.1f;
	}
}

const GLchar *vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 projection;

void main()
{
    vec4 worldPos = model * vec4(position, 1.0);

    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(model))) * normal;

    TexCoord = texCoord;
    gl_Position = projection * worldPos;
}
)";

const GLchar *fragmentShaderSource = R"(
#version 400

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

out vec4 color;

uniform sampler2D texture1;

uniform vec3 lightPos;
uniform vec3 viewPos;

// coeficientes
uniform vec3 Ka; // ambiente
uniform vec3 Kd; // difuso
uniform vec3 Ks; // especular
uniform float shininess;

void main()
{
    vec3 texColor = texture(texture1, TexCoord).rgb;

    // AMBIENTE
    vec3 ambient = Ka * texColor;

    // DIFUSO
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = Kd * diff * texColor;

    // ESPECULAR
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = Ks * spec;

    vec3 result = ambient + diffuse + specular;
    color = vec4(result, 1.0);
}
)";

GLuint setupShader()
{
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertexShaderSource, NULL);
	glCompileShader(vs);

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fragmentShaderSource, NULL);
	glCompileShader(fs);

	GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}


string loadMTL(const string& path)
{
    ifstream file(path);
    string line;
    string textureFile;

    while (getline(file, line))
    {
        stringstream ss(line);
        string type;
        ss >> type;

        if (type == "map_Kd")
        {
            ss >> textureFile;
        }
    }

    return textureFile;
}

struct Vertex {
    vec3 position;
    vec2 texCoord;
    vec3 normal;
};

pair<vector<Vertex>, string> loadOBJ(const string& path)
{
	string mtlFile;
	vector<vec3> positions;
	vector<vec2> texcoords;
	vector<Vertex> vertices;
	vector<vec3> normals;

	ifstream file(path);
	string line;

	if (!file.is_open()) {
		cout << "ERRO: nao abriu OBJ -> " << path << endl;
		return {{}, ""};
	}

	while (getline(file, line))
{
    stringstream ss(line);
    string type;
    ss >> type;

    if (type == "v")
    {
        float x, y, z;
        ss >> x >> y >> z;
        positions.push_back(vec3(x,y,z));
    }
    else if (type == "vt")
    {
        float u, v;
        ss >> u >> v;
        texcoords.push_back(vec2(u,v));
    }
	else if (type == "vn")
	{
		float x, y, z;
		ss >> x >> y >> z;
		normals.push_back(vec3(x,y,z));
	}
    else if (type == "f")
    {
        string v1, v2, v3;
        ss >> v1 >> v2 >> v3;

        vector<string> vs = {v1, v2, v3};

        for (auto &v : vs)
		{
			int vi = 0, ti = 0, ni = 0;

			if (sscanf(v.c_str(), "%d/%d/%d", &vi, &ti, &ni) == 3) {}
			else if (sscanf(v.c_str(), "%d/%d", &vi, &ti) == 2) {}
			else if (sscanf(v.c_str(), "%d", &vi) == 1) {
				ti = 0;
			}

			Vertex vert;

			vert.position = positions[vi - 1];

			if (ti > 0 && ti <= texcoords.size())
			{
				vec2 uv = texcoords[ti - 1];
				uv.x = uv.x - floor(uv.x);
				uv.y = uv.y - floor(uv.y);
				vert.texCoord = uv;
			}
			else
			{
				vert.texCoord = vec2(0.0f, 0.0f);
			}

			if (ni > 0 && ni <= normals.size())
				vert.normal = normals[ni - 1];
			else
				vert.normal = vec3(0.0f, 0.0f, 1.0f);

			vertices.push_back(vert);
		}
    }
    else if (type == "mtllib")
    {
        ss >> mtlFile;
    }
}

	cout << "Vertices carregados: " << vertices.size() << endl;

	return {vertices, mtlFile};
}

GLuint setupGeometry(vector<Vertex>& vertices)
{
	GLuint VBO, VAO;

	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER,
		vertices.size() * sizeof(Vertex),
		vertices.data(),
		GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);

	return VAO;
}

int main()
{
	glfwInit();
	

	GLFWwindow *window = glfwCreateWindow(800, 600, "Adicionando Iluminação - Guilherme Tarrasconi", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_control);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	glEnable(GL_DEPTH_TEST);
	GLuint shader = setupShader();
	glUseProgram(shader);

	glUniform3f(glGetUniformLocation(shader, "lightPos"), 2.0f, 2.0f, 2.0f);
	glUniform3f(glGetUniformLocation(shader, "viewPos"), 0.0f, 0.0f, 3.0f);

	glUniform3f(glGetUniformLocation(shader, "Ka"), 0.2f, 0.2f, 0.2f);
	glUniform3f(glGetUniformLocation(shader, "Kd"), 0.7f, 0.7f, 0.7f);
	glUniform3f(glGetUniformLocation(shader, "Ks"), 1.0f, 1.0f, 1.0f);
	glUniform1f(glGetUniformLocation(shader, "shininess"), 32.0f);

	auto result = loadOBJ("C:/Users/guita/Downloads/Mod-2-Instanciando-objetos-na-cena-3D-main (1)/Mod-2-Instanciando-objetos-na-cena-3D-main/assets/Modelos3D/Cube.obj");
	
	vector<Vertex> vertices = result.first;
	cout << "Vertices carregados: " << vertices.size() << endl;

	if (vertices.empty()) {
		cout << "ERRO: OBJ nao carregado!" << endl;
	}

	string mtlFile = result.second;
	GLuint VAO = setupGeometry(vertices);
	cout << "Vertices carregados: " << vertices.size() << endl;
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_set_flip_vertically_on_load(true);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
	int width, height, nrChannels;	
	unsigned char *data = stbi_load("C:/Users/guita/Downloads/Mod-2-Instanciando-objetos-na-cena-3D-main (1)/Mod-2-Instanciando-objetos-na-cena-3D-main/assets/Modelos3D/brick.png", &width, &height, &nrChannels, 0);

	if (!data)
	{
		cout << "ERRO: textura nao carregou\n";
	}

		if (!data)
	{
		cout << "ERRO: textura nao carregou\n";
	}

	if (data)
	{
		GLenum format;
		if (nrChannels == 3)
			format = GL_RGB;
		else if (nrChannels == 4)
			format = GL_RGBA;

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	stbi_image_free(data);

	glUniform1i(glGetUniformLocation(shader, "texture1"), 0);

	mat4 projection = perspective(radians(45.0f), 800.0f/600.0f, 0.1f, 100.0f);
	glUniformMatrix4fv(glGetUniformLocation(shader,"projection"),1,GL_FALSE,value_ptr(projection));

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(VAO);
		glBindTexture(GL_TEXTURE_2D, texture);

		mat4 model = mat4(1.0f);
		model = translate(model, vec3(0,0,-3));
		model = translate(model, position);
		model = scale(model, vec3(scaleFactor));

		glUniformMatrix4fv(glGetUniformLocation(shader,"model"),1,GL_FALSE,value_ptr(model));

		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}