#include <iostream>
#include "SerialDeviceHost.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <numeric>

#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING)
#define GLFW_INCLUDE_NONE         // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h>  // Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

using namespace rw;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int argc, char *argv[])
{
    // Look for device with ClassID: 0x0A0A and TypeID: 0x0D0E, it will scan all available ports to find a match
    serial_device::SerialDeviceHost myDevice(  0x0A0A, 0x0D0E);

    // Check to make sure the device was found and the port was able to be opened
    if (!myDevice.isOpen() || !myDevice.isFound())
    {
        std::cout << "Error Opening Port or Device Not Found" << std::endl;
        return EXIT_FAILURE;
    }

    // Print Device Info
    std::cout << "Found Device On Port: " << myDevice.getPortName() << std::endl;
    std::cout << "  Description: " << myDevice.getName() << " - " << myDevice.getInfo() << std::endl;
    std::cout << "  Class ID: " << myDevice.getClassID() << std::endl;
    std::cout << "  Type ID: " << myDevice.getTypeID() << std::endl;
    std::cout << "  Serial #: " << myDevice.getSerial() << std::endl;
    std::cout << "  Ver: " << (int)myDevice.getVersionMajor() << "." << (int)myDevice.getVersionMinor() << "." << (int)myDevice.getVersionRev() << std::endl;

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return EXIT_FAILURE;

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1140, 525, "SerialDevice - Multiple IO Example", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
  bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING)
  bool err = false;
    glbinding::initialize([](const char* name) { return (glbinding::ProcAddress)glfwGetProcAddress(name); });
#else
  bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    std::vector<float> photoRes;
    std::vector<float> pot;
    uint32_t trueRNG = 0;
    int32_t encoderCount = 0;
    uint8_t buttonState = 0;
    uint8_t lastButtonState = 0;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Call update to run device routines and process any new data
        myDevice.update();

        // Process new data if available
        if (myDevice.available())
        {
            // Check for Photoresistor data
            if (myDevice.findIndex("pho") != -1)
            {
                auto phoVal = myDevice.get<uint16_t>("pho");
                photoRes.emplace_back(phoVal);
                if (photoRes.size() > 150)
                    photoRes.erase(photoRes.begin());
            }
            // Check for Potentiometer data
            if (myDevice.findIndex("pot") != -1)
            {
                auto potVal = myDevice.get<uint16_t>("pot");
                pot.emplace_back(potVal);
                if (pot.size() > 150)
                    pot.erase(pot.begin());
            }
            // Check for RNG data
            if (myDevice.findIndex("rng") != -1)
            {
                trueRNG = myDevice.get<uint32_t>("rng");
            }
            // Check for Encoder data
            if (myDevice.findIndex("enc"))
            {
                encoderCount = myDevice.get<int32_t>("enc");
            }
            // Check for Button press data
            if (myDevice.findIndex("but") != -1)
            {
                buttonState = myDevice.get<uint8_t>("but");
            }

        }

        // Clear data for sending new packet
        myDevice.clearData();

        // GUI Stuff Here
        {
            ImGui::Begin("Analog Inputs" ); // Start Analog Interface
            ImGui::SetWindowSize(ImVec2(715, 500), ImGuiCond_Once);
            ImGui::SetWindowPos(ImVec2(4, 4), ImGuiCond_Once);

            // Photoresistor
            float average1 = accumulate( photoRes.begin(), photoRes.end(), 0.0)/photoRes.size();
            char overlay1[32];
            sprintf(overlay1, "avg %.2f", average1);
            ImGui::PlotLines("Photoresistor", photoRes.data(), photoRes.size(), 0, overlay1, 0.0f, 4096.0f, ImVec2(0, 200));
            float progress = 0.0f;
            if (!photoRes.empty())
                progress = photoRes.back();
            char buf[32];
            sprintf(buf, "%.2f", progress);
            ImGui::ProgressBar(progress  / 4096.0f, ImVec2(0.f,0.f), buf);
            ImGui::Separator();

            // Potentiometer
            float average2 = accumulate( pot.begin(), pot.end(), 0.0)/pot.size();
            char overlay2[32];
            sprintf(overlay2, "avg %.2f", average2);
            ImGui::PlotLines("Potentiometer", pot.data(), pot.size(), 0, overlay2, 0.0f, 4096.0f, ImVec2(0, 200));
            progress = 0.0f;
            if (!photoRes.empty())
                progress = pot.back();
            sprintf(buf, "%.2f", progress);
            ImGui::ProgressBar(progress  / 4096.0f, ImVec2(0.f,0.f), buf);
            ImGui::Separator();

            ImGui::End(); // End Analog Interface

            // -------------------------------------------------

            ImGui::Begin("Controls"); // Start Controls
            ImGui::SetWindowSize(ImVec2(412, 500), ImGuiCond_Once);
            ImGui::SetWindowPos(ImVec2(722, 4), ImGuiCond_Once);

            // Button and Label for RNG
            if (ImGui::Button("Generate True RNG", ImVec2(-1, 40))) {
                myDevice.add("rnd", "get");
            }
            static char buf1[64] = "";
            sprintf(buf1, "%u", trueRNG);
            static float col1[3] = { 0.0f, 0.0f, 0.0f };
            col1[0] = (float)(((trueRNG >> 16) & 0xFF) / 255.0f);
            col1[1] = (float)(((trueRNG >> 8) & 0xFF) / 255.0f);
            col1[2] = (float)((trueRNG & 0xFF) / 255.0f);
            ImGui::ColorEdit3("##empty", col1);
            ImGui::InputText("RNG", buf1, 64, ImGuiInputTextFlags_ReadOnly);
            ImGui::Separator();

            // LED PWM Slider
            static int pwmVal = 127;
            static int lastVal = 0;
            ImGui::SliderInt("PWM LED", &pwmVal, 1, 255);
            uint8_t tmpVal = pwmVal;
            if (lastVal != pwmVal) {
                myDevice.add("pwm", "set");
                myDevice.add("lvl", tmpVal);
            }
            lastVal = pwmVal;
            ImGui::Separator();

            // Physical Button
            static int counter = 0;
            ImVec4 buttonColor;
            if (buttonState == 0) {
                buttonColor = (ImVec4) ImColor(0, 0, 255, 255);
                lastButtonState = buttonState;
            }
            else {
                buttonColor = (ImVec4) ImColor(0, 255, 0, 255);
                if (buttonState != lastButtonState) {
                    counter++;
                    lastButtonState = buttonState;
                }
            }
            ImGui::PushID(1);
            ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, buttonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonColor);
            ImGui::Button("On Board Button");
            ImGui::PopStyleColor(3);
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::Text("%d", counter);
            ImGui::Separator();

            // On Board LED
            static bool onBoardLED = false;
            static bool lastLedState = false;
            ImGui::Checkbox("On Board LED", &onBoardLED);
            if (onBoardLED != lastLedState) {
                if (onBoardLED) {
                    myDevice.add("led", "set");
                    myDevice.add("sta", (uint8_t) 1);
                } else {
                    myDevice.add("led", "set");
                    myDevice.add("sta", (uint8_t) 0);
                }
            }
            lastLedState = onBoardLED;
            ImGui::Separator();

            // Encoder
            ImGui::Text("Encoder: ");
            ImGui::SameLine();
            ImGui::Text("%i", encoderCount);
            static int lIndex = 0;
            static int lastEncodeVal = encoderCount;
            lIndex += (encoderCount - lastEncodeVal);
            if (lIndex > 9)
                lIndex = 0;
            else if (lIndex < 0)
                lIndex = 9;

            ImGui::ListBoxHeader("##empty", ImVec2(-1, 200));
            for (int i = 0; i < 10; i++)
            {
                std::string lbl = "Item";
                lbl += std::to_string(i + 1);
                if (lIndex == i)
                    ImGui::Selectable(lbl.c_str(), true);
                else
                    ImGui::Selectable(lbl.c_str(), false);
            }
            ImGui::ListBoxFooter();
            lastEncodeVal = encoderCount;

            ImGui::End(); // End Controls

            // -------------------------------------------------

            // Send Packet data
            myDevice.sendPacket();

            // Rendering GUI
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }

    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
