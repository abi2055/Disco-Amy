#define TINYOBJLOADER_IMPLEMENTATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tiny_obj_loader.h>

#include <fstream>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// definitions required 

// Adjusting the window size
 void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
   glViewport(0, 0, width, height);
 }

// Provided function to screenshot renders
static unsigned int ss_id = 0;
void dump_framebuffer_to_ppm(std::string prefix, unsigned int width,
                             unsigned int height) {
  int pixelChannel = 3;
  int totalPixelSize = pixelChannel * width * height * sizeof(GLubyte);
  GLubyte* pixels = new GLubyte[totalPixelSize];
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
  std::string file_name = prefix + std::to_string(ss_id) + ".ppm";
  std::ofstream fout(file_name);
  fout << "P3\n" << width << " " << height << "\n" << 255 << std::endl;
  for (size_t i = 0; i < height; i++) {
    for (size_t j = 0; j < width; j++) {
      size_t cur = pixelChannel * ((height - i - 1) * width + j);
      fout << (int)pixels[cur] << " " << (int)pixels[cur + 1] << " "
           << (int)pixels[cur + 2] << " ";
    }
    fout << std::endl;
  }
  ss_id++;
  delete[] pixels;
  fout.flush();
  fout.close();
}

// Given function to handle screenshotting
void processInput(GLFWwindow* window) {
  // press escape to exit
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  // press p to capture screen
  if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
    std::cout << "Capture Window " << ss_id << std::endl;
    int buffer_width, buffer_height;
    glfwGetFramebufferSize(window, &buffer_width, &buffer_height);
    dump_framebuffer_to_ppm("screenshot-ss", buffer_width, buffer_height);
  }
}

// Given Load Model Function
void loadModel(const std::string& path, std::vector<float>& vbuffer, std::vector<float>& nbuffer, std::vector<float>& tbuffer) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str());

  for (const auto& id : shapes[0].mesh.indices) {
    int vid = id.vertex_index;
    int nid = id.normal_index;
    int tid = id.texcoord_index;

    // Fill in vertex positions
    vbuffer.push_back(attrib.vertices[vid * 3]);
    vbuffer.push_back(attrib.vertices[vid * 3 + 1]);
    vbuffer.push_back(attrib.vertices[vid * 3 + 2]);

    // Fill in normals
    nbuffer.push_back(attrib.normals[nid * 3]);
    nbuffer.push_back(attrib.normals[nid * 3 + 1]);
    nbuffer.push_back(attrib.normals[nid * 3 + 2]);

    // Fill in texture coordinates
    tbuffer.push_back(attrib.texcoords[tid * 2]);
    tbuffer.push_back(attrib.texcoords[tid * 2 + 1]);
  }
}

// Given load texture Function
unsigned int loadTexture(const char* path) {
  unsigned int textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  int width, height, nrChannels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);
      stbi_image_free(data);
      return textureID;
}

// Given Vertex Shader Source 
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoord;
uniform mat4 MVP;
void main() {
  FragPos = aPos;
  Normal = aNormal;
  TexCoord = aTexCoord;
  gl_Position = MVP * vec4(aPos, 1.0);
}
)";

// Given Fragment Shader Source
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

out vec4 FragColor;
uniform vec3 objColor;
uniform sampler2D myTexture;

// Variables for each colour spot light: spotlight position, spotlight direction, diffuse light, ambient light 
uniform vec3 lightPosRed, spotDirRed, diffuseColorRed, ambientColorRed;
uniform vec3 lightPosGreen, spotDirGreen, diffuseColorGreen, ambientColorGreen;
uniform vec3 lightPosBlue, spotDirBlue, diffuseColorBlue, ambientColorBlue;

// cutOffCosine defines the radius of the spotlight
uniform float cutOffCosine;

// Given Function to calculate spotlight information 
vec3 calculateSpotlight(vec3 lightPos, vec3 spotDir, vec3 diffuseColor) {
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(lightPos - FragPos);
  float theta = dot(lightDir, normalize(-spotDir));

   if (theta > cutOffCosine) {
        float distance = length(lightPos - FragPos);
        // attenutation specified by assignment 
        float attenuation = 1.0 / (1.0 + 0.000035 * distance + 0.000044 * distance * distance);
        float diff = max(dot(norm, lightDir), 0.0);
        
        // Combining all types of lighting to return the diffuse of each color
        return diffuseColor * objColor * diff * attenuation;
        
    }
    return vec3(0.0);
}

// Given main function but additions made 
void main() {

  // Ambient lighting of each spotlight 
  vec3 ambientRed = ambientColorRed * objColor;
  vec3 ambientGreen = ambientColorGreen * objColor;
  vec3 ambientBlue = ambientColorBlue * objColor;

  // Making function calls to calculate lighting information needed for each spotlight + the ambient light of each 
  // to light up the render 
  vec3 diffuseRed = calculateSpotlight(lightPosRed, spotDirRed, diffuseColorRed) + ambientRed;
  vec3 diffuseGreen = calculateSpotlight(lightPosGreen, spotDirGreen, diffuseColorGreen) + ambientGreen;
  vec3 diffuseBlue = calculateSpotlight(lightPosBlue, spotDirBlue, diffuseColorBlue) + ambientBlue;

  // Putting everything together in a result 
  vec3 result = diffuseRed + diffuseGreen + diffuseBlue;

  FragColor = texture(myTexture, TexCoord) * vec4(result, 1.0);
}
)";

// shader program from past assignment 
unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource) {
  int success;
  char error_msg[512];
  unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vertexShaderSource, NULL);
  glCompileShader(vs);
  glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vs, 512, NULL, error_msg);
    std::cout << "Vertex Shader Failed: " << error_msg << std::endl;
  }
  unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fragmentShaderSource, NULL);
  glCompileShader(fs);
  glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fs, 512, NULL, error_msg);
    std::cout << "Fragment Shader Failed: " << error_msg << std::endl;
  }
  unsigned int shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vs);
  glAttachShader(shaderProgram, fs);
  glLinkProgram(shaderProgram);
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, error_msg);
    std::cout << "Program Link Error: " << error_msg << std::endl;
  }
  glDeleteShader(vs);
  glDeleteShader(fs);
  return shaderProgram;
}

int main() {

  // provided setup 
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Specified window size as per assignment 
  GLFWwindow* window = glfwCreateWindow(1024, 512, "Amy", NULL, NULL);
  
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  std::cout << "Initializing GLAD..." << std::endl;
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD!" << std::endl;
    return -1;
  }

  std::cout << "Initialization completed successfully." << std::endl;

  // Given openGL Assignment Specifications 
  glClearColor(0.3f, 0.4f, 0.5f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  // Given Setup Function 
  auto setupVAO = [](std::vector<float>& vbuffer, std::vector<float>& nbuffer,
                     std::vector<float>& tbuffer, unsigned int& VAO,
                     unsigned int* VBO) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(3, VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, vbuffer.size() * sizeof(float),
                 vbuffer.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, nbuffer.size() * sizeof(float),
                 nbuffer.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, tbuffer.size() * sizeof(float),
                 tbuffer.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(2);
  };

  // (1) Load Amy and Texture
  std::vector<float> amyVBuffer, amyNBuffer, amyTBuffer;
  loadModel("./asset/Amy.obj", amyVBuffer, amyNBuffer, amyTBuffer);

  unsigned int amyTexture = loadTexture("./asset/Amy.png");

  unsigned int amyVAO, amyVBO[3];
  setupVAO(amyVBuffer, amyNBuffer, amyTBuffer, amyVAO, amyVBO);

  // (2) Load Bucket and Texture
  std::vector<float> bucketVBuffer, bucketNBuffer, bucketTBuffer;
  loadModel("./asset/bucket.obj", bucketVBuffer, bucketNBuffer, bucketTBuffer);

  unsigned int bucketTexture = loadTexture("./asset/bucket.jpg");

  unsigned int bucketVAO, bucketVBO[3];
  setupVAO(bucketVBuffer, bucketNBuffer, bucketTBuffer, bucketVAO, bucketVBO);

  // (3) Load Floor and Texture
  std::vector<float> floorVBuffer, floorNBuffer, floorTBuffer;
  loadModel("./asset/floor.obj", floorVBuffer, floorNBuffer, floorTBuffer);

  unsigned int floorTexture = loadTexture("./asset/floor.jpg");

  unsigned int floorVAO, floorVBO[3];
  setupVAO(floorVBuffer, floorNBuffer, floorTBuffer, floorVAO, floorVBO);

  // Creating the shader program
  unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

  // base color of objects
  glm::vec3 objColor(1.0f, 1.0f, 1.0f); 

  // Red Spotlight Configuration
  glm::vec3 lightPosRed(0.0f, 200.0f, 0.0f);
  glm::vec3 spotDirRed(50.0f, -200.0f, -50.0f);
  glm::vec3 diffuseColorRed(1.0f, 0.0f, 0.0f);
  glm::vec3 ambientColorRed(0.2f, 0.0f, 0.0f); 

  // Green Spotlight Configuration
  glm::vec3 lightPosGreen(0.0f, 200.0f, 0.0f);
  glm::vec3 spotDirGreen(-50.0f, -200.0f, -50.0f);
  glm::vec3 diffuseColorGreen(0.0f, 1.0f, 0.0f);
  glm::vec3 ambientColorGreen(0.0f, 0.2f, 0.0f); 

  // Blue Spotlight Configuration
  glm::vec3 lightPosBlue(0.0f, 200.0f, 0.0f);
  glm::vec3 spotDirBlue(0.0f, -200.0f, 50.0f);
  glm::vec3 diffuseColorBlue(0.0f, 0.0f, 1.0f);
  glm::vec3 ambientColorBlue(0.0f, 0.0f, 0.2f);  

  float cutOffCosine = glm::cos(glm::pi<float>() / 6.0f); 
  // Cut off Angle given 

  glUseProgram(shaderProgram);

  glUniform3fv(glGetUniformLocation(shaderProgram, "objColor"), 1, glm::value_ptr(objColor));

  // Lighting information to shader program for red
  glUniform3fv(glGetUniformLocation(shaderProgram, "lightPosRed"), 1, glm::value_ptr(lightPosRed));
  glUniform3fv(glGetUniformLocation(shaderProgram, "spotDirRed"), 1, glm::value_ptr(spotDirRed));
  glUniform3fv(glGetUniformLocation(shaderProgram, "diffuseColorRed"), 1, glm::value_ptr(diffuseColorRed));
  glUniform3fv(glGetUniformLocation(shaderProgram, "ambientColorRed"), 1, glm::value_ptr(ambientColorRed));

  // Lighting information to shader program for green
  glUniform3fv(glGetUniformLocation(shaderProgram, "lightPosGreen"), 1, glm::value_ptr(lightPosGreen));
  glUniform3fv(glGetUniformLocation(shaderProgram, "spotDirGreen"), 1, glm::value_ptr(spotDirGreen));
  glUniform3fv(glGetUniformLocation(shaderProgram, "diffuseColorGreen"), 1, glm::value_ptr(diffuseColorGreen));
  glUniform3fv(glGetUniformLocation(shaderProgram, "ambientColorGreen"), 1, glm::value_ptr(ambientColorGreen));

  // Lighting information to shader program for blue
  glUniform3fv(glGetUniformLocation(shaderProgram, "lightPosBlue"), 1, glm::value_ptr(lightPosBlue));
  glUniform3fv(glGetUniformLocation(shaderProgram, "spotDirBlue"), 1, glm::value_ptr(spotDirBlue));
  glUniform3fv(glGetUniformLocation(shaderProgram, "diffuseColorBlue"), 1, glm::value_ptr(diffuseColorBlue));
  glUniform3fv(glGetUniformLocation(shaderProgram, "ambientColorBlue"), 1, glm::value_ptr(ambientColorBlue));

  // Cutoff information to shader program
  glUniform1f(glGetUniformLocation(shaderProgram, "cutOffCosine"), cutOffCosine);

  // Transformation matrices, given by assignment specifications 
  glm::mat4 model = glm::mat4(1.0f);
  glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 100.0f, 180.0f), glm::vec3(0.0f, 80.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 projection = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f);

   // Solid rendering 
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   // angle for rotation of spotlight
   float theta = 0.0f;

  while (!glfwWindowShouldClose(window)) {
    // Given setup 
    processInput(window);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "myTexture"), 0);
    glActiveTexture(GL_TEXTURE0);

    // Amy Render
    glBindTexture(GL_TEXTURE_2D, amyTexture);
    glBindVertexArray(amyVAO);
    glm::mat4 amyModel = glm::mat4(1.0f);
    glm::mat4 amyMVP = projection * view * amyModel;
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"), 1, GL_FALSE,glm::value_ptr(amyMVP));
    glDrawArrays(GL_TRIANGLES, 0, amyVBuffer.size() / 3);

    // Floor render
    glBindTexture(GL_TEXTURE_2D, floorTexture);
    glBindVertexArray(floorVAO);
    glm::mat4 floorModel = glm::mat4(1.0f);
    glm::mat4 floorMVP = projection * view * floorModel;
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"), 1, GL_FALSE, glm::value_ptr(floorMVP));
    glDrawArrays(GL_TRIANGLES, 0, floorVBuffer.size() / 3);
    
    // Bucket Render
    glBindTexture(GL_TEXTURE_2D, bucketTexture); 
    glBindVertexArray(bucketVAO);         
    glm::mat4 bucketModel = glm::mat4(1.0f);
    glm::mat4 bucketMVP = projection * view * bucketModel;
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"), 1, GL_FALSE, glm::value_ptr(bucketMVP));
    glDrawArrays(GL_TRIANGLES, 0, bucketVBuffer.size() / 3);

     // need to rotate around y axis therefore takes in changing theta every loop and rotation vector (0, 1, 0)
    glm::mat4 RM = glm::rotate(glm::mat4(1.0f), theta, glm::vec3(0.0f, 1.0f, 0.0f));

    // Rotation of spotlights, needs to be a 4x4 matrix for multiplying transformation matrices
    glm::vec3 ChangingSpotDirRed =  glm::vec3(RM * glm::vec4(spotDirRed, 1.0f));
    glm::vec3 ChangingSpotDirGreen = glm::vec3(RM * glm::vec4(spotDirGreen, 1.0f));
    glm::vec3 ChangingSpotDirBlue = glm::vec3(RM * glm::vec4(spotDirBlue, 1.0f));

    // Inputting the changing spotlight into the shader to update location 
    glUseProgram(shaderProgram);
    glUniform3fv(glGetUniformLocation(shaderProgram, "spotDirRed"), 1, glm::value_ptr(ChangingSpotDirRed));
    glUniform3fv(glGetUniformLocation(shaderProgram, "spotDirGreen"), 1, glm::value_ptr(ChangingSpotDirGreen));
    glUniform3fv(glGetUniformLocation(shaderProgram, "spotDirBlue"), 1, glm::value_ptr(ChangingSpotDirBlue));

    // changing theta, I made it 0.025 since it was moving to fast for the recording (it is 0.05f)
    theta += 0.025f;  

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
