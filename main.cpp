#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/GL.h>
#include <tchar.h>

#include "image_handler.h"
#include "image_processor.h"
#include "ui_helper.h"

// Data stored per platform window
struct WGL_WindowData { HDC hDC; };

// Constants
static const POINT      k_WindowSize = {400, 500};
enum UIPage { SELECT_FILE, HIDE_MESSAGE, RETRIEVE_MESSAGE, ANALYZE };

// UI Data
static HGLRC            g_hRC;
static WGL_WindowData   g_MainWindow;
static int              g_Width;
static int              g_Height;
static bool             g_MouseDownOnHeader;
static POINT            g_LastMousePos;

// Program Data
std::string             m_ImageInputPath;
std::string             m_ImageOutputPath;
ImageDetails            m_ImageDetails;
UIPage                  m_UIPage = SELECT_FILE;
bool                    m_ResetData = false;

// Forward declarations of helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);
void ResetDeviceWGL();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int main(int, char**)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"crypng", nullptr };
    ::RegisterClassExW(&wc);

    RECT RectWindow;
	GetWindowRect(GetDesktopWindow(), &RectWindow);
	int WindowStartX = RectWindow.right / 2 - (k_WindowSize.x / 2);
	int WindowStartY = RectWindow.bottom / 2 - (k_WindowSize.y / 2);

    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"crypng", WS_POPUP, WindowStartX, WindowStartY, k_WindowSize.x, k_WindowSize.y, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize OpenGL
    if (!CreateDeviceWGL(hwnd, &g_MainWindow))
    {
        CleanupDeviceWGL(hwnd, &g_MainWindow);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    wglMakeCurrent(g_MainWindow.hDC, g_hRC);

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
    io.IniFilename = NULL;

    // Setup Dear ImGui style
    ImGui::StyleColorsClassic();
    //ImGui::StyleColorsClassic();

    // Modify ImGui Style Defaults
    ImGuiStyle& style = ImGui::GetStyle();
    //style.TabRounding = 8.f;
    style.FrameRounding = 2.f;
    //style.GrabRounding = 8.f;
    style.WindowRounding = 8.f;
    //style.PopupRounding = 8.f;
    style.ChildRounding = 2.f;
    style.AntiAliasedLines = false;
    style.AntiAliasedFill = false;

    // Dear ImGui color theme attribution: https://gist.github.com/enemymouse/c8aa24e247a1d7b9fc33d45091cbb8f0
    style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.00f, 0.40f, 0.41f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 1.00f, 1.00f, 0.65f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.44f, 0.80f, 0.80f, 0.18f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.44f, 0.80f, 0.80f, 0.27f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.44f, 0.81f, 0.86f, 0.66f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.18f, 0.21f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.28f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.22f, 0.29f, 0.30f, 0.71f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.00f, 1.00f, 1.00f, 0.44f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 1.00f, 1.00f, 0.68f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 1.00f, 1.00f, 0.36f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.76f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.00f, 0.65f, 0.65f, 0.46f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.01f, 1.00f, 1.00f, 0.43f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.62f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.00f, 1.00f, 1.00f, 0.32f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.42f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 1.00f, 1.00f, 0.22f);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL3_Init();

    LONG lStyle = GetWindowLong( hwnd, GWL_STYLE );

    SetWindowLong( hwnd, GWL_EXSTYLE, GetWindowLong( hwnd, GWL_STYLE ) | WS_EX_LAYERED);

    SetLayeredWindowAttributes(hwnd, COLORREF(RGB(255, 0, 0)), 255, LWA_COLORKEY | LWA_ALPHA);

    // Our state
    ImVec4 clear_color = ImVec4(1.0f, 0.0f, 0.0f, 1.00f);

    // Program specific initializations
    InitializeRandomSeed();

    // Main loop
    bool done = false;

    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;
        if (::IsIconic(hwnd))
        {
            ::Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        static DWORD dwFlag = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;
        static bool open = true;

		if (!open)
			ExitProcess(0);

        ImGui::SetNextWindowSize(ImVec2(k_WindowSize.x, k_WindowSize.y));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin("crypng", &open, dwFlag);

        if (ImGui::IsKeyPressed(ImGuiKey_F4)) m_UIPage = UIPage::ANALYZE;

        if (m_UIPage == SELECT_FILE)
        {
            if (m_ImageInputPath == "")
            {
                ImGuiDisplayLogo();
                ImGui::Text("Thanks for using crypng by Kyle Meyer");
                ImGui::Text("Select a PNG image to get started");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 1.0f, 1.0f), "[F4] - Analyze");
                if (ImGui::Button("Select Image", ImVec2(ImGui::GetContentRegionAvail().x, 32.0f)))
                {
                    m_ImageDetails = ImageDetails{};
                    OpenFileDialog(m_ImageInputPath, hwnd);
                }
            }
            else
            {
                LoadDataFromFile(m_ImageInputPath, m_ImageDetails);

                ImGuiDisplayImage(m_ImageDetails);

                ImGui::Text("%s | %dx%d px | %s", UIHelper::ClampFileName(m_ImageDetails.name, 20).c_str(), m_ImageDetails.width, m_ImageDetails.height, UIHelper::ChannelCountToDescriptor(m_ImageDetails.channels).c_str());
                ImGui::SetCursorPos(ImVec2(8.0f, 456.0f));
                ImGui::Separator();
                if (ImGui::Button("Select Another Image", ImVec2(0.0f, 32.0f)))
                {
                    m_ImageDetails = ImageDetails{};
                    OpenFileDialog(m_ImageInputPath, hwnd);
                    m_ResetData = true;
                }
                ImGui::SameLine(0.0f, 16.0f);
                if (ImGui::Button("Encode Message", ImVec2(0.0f, 32.0f)))
                {
                    m_UIPage = HIDE_MESSAGE;
                }
                ImGui::SameLine();
                if (ImGui::Button("Decode Message", ImVec2(0.0f, 32.0f)))
                {
                    m_UIPage = RETRIEVE_MESSAGE;
                }
            }
        }
        else if (m_UIPage == HIDE_MESSAGE)
        {
            static char message_buf[65536] = { 0x00 };
            static unsigned char private_key[AES_KEYLEN];
            static bool complete = false;
            
            if (m_ResetData)
            {
                m_ResetData = false;
                memset(message_buf, 0x00, 65536);
                memset(private_key, 0x00, AES_KEYLEN);
                complete = false;
            }

            ImGui::Text("Enter a message to hide:");
            ImGui::InputTextMultiline("##Secret Text", message_buf, m_ImageDetails.max_chars, ImVec2(ImGui::GetContentRegionAvail().x, 192.0f));
            ImGui::Text("Characters Remaining: %d", (m_ImageDetails.max_chars - strlen(message_buf)));

            if (ImGui::Button("Encode", ImVec2(0.0f, 32.0f)))
            {
                complete = false;
                XCrypt::ThreadPerformEncryptionPipeline(message_buf, strlen(message_buf), private_key, AES_KEYLEN, m_ImageDetails, complete);
            }
            if (complete)
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Encryption Completed");

            UIHelper::ImGuiDisplayKeyPhrase(private_key, AES_KEYLEN);

            ImGui::SetCursorPos(ImVec2(8.0f, 456.0f));
            ImGui::Separator();
            if (ImGui::Button("Back", ImVec2(0.0f, 32.0f)))
            {
                m_ImageDetails = ImageDetails{};
                m_UIPage = SELECT_FILE;
            }

            if (complete)
            {
                ImGui::SameLine(0.0f, 270.0f);
                if (ImGui::Button("Save Image", ImVec2(0.0f, 32.0f)))
                {
                    SaveFileDialog(m_ImageOutputPath, hwnd);
                    SaveDataToFile(m_ImageOutputPath, m_ImageDetails);
                }
            }
        }
        else if (m_UIPage == RETRIEVE_MESSAGE)
        {
            static char message_buf[65536];
            static unsigned char private_key[AES_KEYLEN];
            static int message_length = 0;
            static bool complete = false;
            
            if (m_ResetData)
            {
                m_ResetData = false;
                memset(message_buf, 0x00, 65536);
                memset(private_key, 0x00, AES_KEYLEN);
                message_length = 0;
                complete = false;
            }

            ImGui::Text("Enter the key phrase:");
            UIHelper::ImGuiInputKeyPhrase(private_key, AES_KEYLEN);

            if (ImGui::Button("Decode Message", ImVec2(0.0f, 32.0f)))
            {
                complete = false;
                XCrypt::ThreadPerformDecryptionPipeline(message_buf, message_length, private_key, AES_KEYLEN, m_ImageDetails, complete);
            }

            if (message_length)
            {
                ImGui::Separator();
                ImGui::Text("Decoded Message:");
                ImGui::BeginChild("##MessageDisplay", ImVec2(0.0f, 128.0f), ImGuiChildFlags_Border);
                ImGui::Text(message_buf);
                ImGui::EndChild();
                if (ImGui::Button("Copy Message", ImVec2(0.0f, 32.0f)))
                    ImGui::SetClipboardText(message_buf);
                ImGui::SameLine();
                ImGui::Text("Note: Decoded text may be longer than\nmessage shown above");
            }

            ImGui::SetCursorPos(ImVec2(8.0f, 456.0f));
            ImGui::Separator();
            if (ImGui::Button("Back", ImVec2(0.0f, 32.0f)))
            {
                m_ImageDetails = ImageDetails{};
                m_UIPage = SELECT_FILE;
            }
        }
        else if (m_UIPage == ANALYZE)
        {
            static std::string analyize_image_path = "";
            static std::string analyze_image_savepath = "";
            static ImageDetails analyze_id;

            ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "[Analysis Menu] Select an Image and Operation:");
            if (ImGui::Button("Select Image", ImVec2(ImGui::GetContentRegionAvail().x, 32.0f)))
            {
                analyze_id = ImageDetails{};
                OpenFileDialog(analyize_image_path, hwnd);
            }
            if (ImGui::CollapsingHeader("View n-th bit per-channel bit plane"))
            {
                static int selected_channel = 0;
                static int selected_bit = 0;
                static bool composite = false;
                ImGui::SliderInt("Channel", &selected_channel, 0, 3);
                ImGui::SliderInt("Bit Index", &selected_bit, 0, 7);
                ImGui::Checkbox("Composite", &composite);
                if (ImGui::Button("Compute & Display Image##0"))
                {
                    analyze_id.data = NULL;
                    LoadDataFromFile(analyize_image_path, analyze_id);
                    LSBtoMSBChannelNthBit(analyze_id, selected_channel, composite, selected_bit);
                    analyze_id.data_id += 1;
                }
                ImGuiDisplayImage(analyze_id);
            }
            if (ImGui::CollapsingHeader("Options"))
            {
                if (ImGui::Button("Back", ImVec2(0.0f, 32.0f)))
                {
                    m_ImageDetails = ImageDetails{};
                    m_UIPage = SELECT_FILE;
                }

                ImGui::SameLine(0.0f, 270.0f);
                if (ImGui::Button("Save Image", ImVec2(0.0f, 32.0f)))
                {
                    SaveFileDialog(analyze_image_savepath, hwnd);
                    SaveDataToFile(analyze_image_savepath, analyze_id);
                }
            }
        }

        
        ImGui::End();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, g_Width, g_Height);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Present
        ::SwapBuffers(g_MainWindow.hDC);
    }

    free(m_ImageDetails.data);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceWGL(hwnd, &g_MainWindow);
    wglDeleteContext(g_hRC);
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    HDC hDc = ::GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    const int pf = ::ChoosePixelFormat(hDc, &pfd);
    if (pf == 0)
        return false;
    if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
        return false;
    ::ReleaseDC(hWnd, hDc);

    data->hDC = ::GetDC(hWnd);
    if (!g_hRC)
        g_hRC = wglCreateContext(data->hDC);
    return true;
}

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    wglMakeCurrent(nullptr, nullptr);
    ::ReleaseDC(hWnd, data->hDC);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool mousedown = false;
POINT lastLocation;
POINT currentpos;

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            g_Width = LOWORD(lParam);
            g_Height = HIWORD(lParam);
        }
        return 0;
    case WM_LBUTTONDOWN:
    {
        GetCursorPos(&g_LastMousePos);
        RECT RectWindow;
        GetWindowRect(hWnd, &RectWindow);
        if (g_LastMousePos.y - RectWindow.top < 16)
        {
            g_MouseDownOnHeader = true;
        }
        break;
    }
    case WM_LBUTTONUP:
    {
        g_MouseDownOnHeader = false;
        break;
    }
    case WM_MOUSEMOVE:
    {
        if (g_MouseDownOnHeader)
        {
            POINT CurrentMousePoint;
            GetCursorPos(&CurrentMousePoint);
            int xDiff = CurrentMousePoint.x - g_LastMousePos.x;
            int yDiff = CurrentMousePoint.y - g_LastMousePos.y;
            
            RECT RectWindow;
            GetWindowRect(hWnd, &RectWindow);

            SetWindowPos(hWnd, nullptr, RectWindow.left + xDiff, RectWindow.top + yDiff, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

            g_LastMousePos = CurrentMousePoint;
        }
        break;
    }
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
