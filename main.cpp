#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <fstream>

void onFrameBufferSize(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void tick();
int compileVertexShader(const char* source);
int compileFragmentShader(const char* source);
int linkShaders(const int vertexShader, const int fragmentShader);

// settings
static const unsigned int SCREEN_WIDTH = 128;
static const unsigned int SCREEN_HEIGHT = 72;

const char* VERTEX_SHADER_SRC = 
"#version 330 core\n"
"layout(location = 0) in vec3 aPos;\n"
"layout(location = 1) in vec3 aCol;\n"
"layout(location = 2) in vec2 aUv;\n"
"out vec3 outCol;\n"
"out vec2 outUv;\n"
"void main() {\n"
	"gl_Position = vec4(aPos, 1.0);\n"
	"outCol = aCol;\n"
	"outUv = aUv;\n"
"}\n";

const char* FRAGMENT_SHADER_SRC =
"#version 330 core\n"
"in vec3 outCol;\n"
"in vec2 outUv;\n"
"out vec4 FragColor;\n"
"uniform sampler2D tex;\n"
"void main() {\n"
	"FragColor = texture(tex, outUv) * vec4(outCol, 1.0);\n"
"};\n";

struct Sprite {
	unsigned char x;
	unsigned char y;
};

std::vector<Sprite*> sprites;
std::vector<Sprite*> spritesForScanline;
Sprite testSprite;

bool left;
bool right;
bool up;
bool down;

int main()
{
	testSprite.x = 24;
	testSprite.y = 8;

	sprites.push_back(&testSprite);

	long size = 3 * SCREEN_WIDTH * SCREEN_HEIGHT;
	char* buffer = new char[size];
	std::ifstream infile("D:\\GitHub\\alien8\\Alien8\\misc\\testscene.bmp");
	infile.seekg(0, infile.end);
	size_t length = infile.tellg();
	infile.seekg(53, infile.beg);

	if (length > size)
	{
		length = size;
	}

	infile.read(buffer, length);


	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH*4, SCREEN_HEIGHT*4, "Alien 8", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, onFrameBufferSize);

	//Load all function pointers for GL stuff with glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	//Compile the shaders and link them
	int vertexShader = compileVertexShader(VERTEX_SHADER_SRC);
	int fragmentShader = compileFragmentShader(FRAGMENT_SHADER_SRC);
	int shaderProgram = linkShaders(vertexShader, fragmentShader);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	float vertices[] = {
		-1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, //Top left
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, //Top right
		 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, //Bottom right
		 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, //Bottom right
     	-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, //Bottom left
		-1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f  //Top left
	};

	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Let's set the attribute pointers for vertices, colors and texture coords
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	// uncomment this call to draw in wireframe polygons.
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	//Generate a texture
	unsigned int screenTex;
	glGenTextures(1, &screenTex);
	glBindTexture(GL_TEXTURE_2D, screenTex);
	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	const int imageDataLength = SCREEN_WIDTH * SCREEN_HEIGHT * 4; //4 because RGBA components
	unsigned char* imageData = new unsigned char[imageDataLength];

	/*int bufPos = 1;
	for (int i = 0; i < imageDataLength; i += 4) {
		imageData[i] = buffer[bufPos + 2];
		imageData[i + 1] = buffer[bufPos + 1];
		imageData[i + 2] = buffer[bufPos];
		imageData[i + 3] = 255;

		bufPos += 3;
		if (bufPos % (387) == 0) {
			bufPos += 1;
		}
	}*/

	//Initialize imageData
	for (int i = 0; i < imageDataLength; i += 4) {
		imageData[i] = 0;
		imageData[i + 1] = 0;
		imageData[i + 2] = 0;
		imageData[i + 3] = 255;
	}

	while (!glfwWindowShouldClose(window))
	{
		processInput(window);
		tick();

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
		glGenerateMipmap(GL_TEXTURE_2D);

		//Re-paint the screen
		for (int i = 0; i < imageDataLength; i += 4) {
			int pixelIndex = i / 4;
			int x = pixelIndex % SCREEN_WIDTH;
			int y = pixelIndex / SCREEN_WIDTH;

			imageData[i] = 0;
			imageData[i + 1] = 0;
			imageData[i + 2] = 0;
			imageData[i + 3] = 255;

			//See if any sprites need to be drawn this scanline
			if (x == 0) {
				spritesForScanline.clear();

				for (auto const& value : sprites) {
					if (value->y >= y && value->y <= y + 8) {
						spritesForScanline.push_back(value);
					}
				}
			}

			//Draw sprites from current scanline
			for (auto const& value : spritesForScanline) {
				if (value->x >= x && value->x <= x + 8) {
					imageData[i + 1] = 255; //Make it green, for now
				}
			}

			//std::cout << "x" << x << "\ny" << y << std::endl;
		}

		glUseProgram(shaderProgram);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	glfwTerminate();
	delete[] imageData;

	return 0;
}

int compileVertexShader(const char* source) {
	int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &source, NULL);
	glCompileShader(vertexShader);

	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	return vertexShader;
}

int compileFragmentShader(const char* source) {
	int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &source, NULL);
	glCompileShader(fragmentShader);

	int success;
	char infoLog[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	return fragmentShader;
}

int linkShaders(const int vertexShader, const int fragmentShader) {
	int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	int success;
	char infoLog[512];
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}

	return shaderProgram;
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	int leftKey = glfwGetKey(window, GLFW_KEY_A);
	int rightKey = glfwGetKey(window, GLFW_KEY_D);
	int upKey = glfwGetKey(window, GLFW_KEY_W);
	int downKey = glfwGetKey(window, GLFW_KEY_S);

	if (leftKey == GLFW_PRESS) {
		left = true;
	}
	if (leftKey == GLFW_RELEASE) {
		left = false;
	}

	if (rightKey == GLFW_PRESS) {
		right = true;
	}
	if (rightKey == GLFW_RELEASE) {
		right = false;
	}

	if (upKey == GLFW_PRESS) {
		up = true;
	}
	if (upKey == GLFW_RELEASE) {
		up = false;
	}

	if (downKey == GLFW_PRESS) {
		down = true;
	}
	if (downKey == GLFW_RELEASE) {
		down = false;
	}
}

void tick() {
	if (up) {
		testSprite.y -= 1;
	}

	if (down) {
		testSprite.y += 1;
	}

	if (left) {
		testSprite.x -= 1;
	}

	if (right) {
		testSprite.x += 1;
	}
}

void onFrameBufferSize(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}