
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <algorithm>
#include <cstdio>
#include <cstring>
using namespace std;
#pragma comment(lib, "ws2_32.lib")

const int COMMAND_PORT = 45678;
const int DISCOVERY_PORT = 45679;

void keyboard(WORD key)
{
    INPUT input[2] = {};

    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = key;

    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = key;
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(2, input, sizeof(INPUT));
}

void mouseClick()
{
    INPUT input[2] = {};

    input[0].type = INPUT_MOUSE;
    input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    input[1].type = INPUT_MOUSE;
    input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(2, input, sizeof(INPUT));
}



void moveMouse(int dx, int dy)
{
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;

    SendInput(1, &input, sizeof(INPUT));
}

void rightClick()
{
    INPUT inputs[2] = {};

    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;

    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;

    SendInput(2, inputs, sizeof(INPUT));
}

void scrollMouse(int amount)
{
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = amount;

    SendInput(1, &input, sizeof(INPUT));
}


void zoomBrowser(bool zoomIn)
{
    INPUT inputs[3] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;

    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_WHEEL;
    inputs[1].mi.mouseData = zoomIn ? 120 : (DWORD)-120;

    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = VK_CONTROL;
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(3, inputs, sizeof(INPUT));
}

void resetZoom()
{
    INPUT inputs[4] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = '0';

    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = '0';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(4, inputs, sizeof(INPUT));
}


void executeCommand(const std::string& cmd)
{
    if (cmd == "NEXT")
        keyboard(VK_RIGHT);

    else if (cmd == "PREV")
        keyboard(VK_LEFT);

    else if (cmd == "ESC")
        keyboard(VK_ESCAPE);

    else if (cmd == "SPACE")
        keyboard(VK_SPACE);

    else if (cmd == "BLACK")
        keyboard('B');

    else if (cmd == "CLICK")
        mouseClick();

    else if (cmd == "RIGHT_CLICK")
        rightClick();

    else if (cmd == "DOUBLE_CLICK") {
        mouseClick();
        Sleep(80);
        mouseClick();
    }

    else if (cmd == "ZOOM_IN")
        zoomBrowser(true);

    else if (cmd == "ZOOM_OUT")
        zoomBrowser(false);

    else if (cmd == "ZOOM_RESET")
        resetZoom();

    else if (cmd.rfind("MOVE:", 0) == 0) {
        int dx = 0, dy = 0;

        if (sscanf(cmd.c_str(), "MOVE:%d:%d", &dx, &dy) == 2) {
            dx = max(-500, min(500, dx));
            dy = max(-500, min(500, dy));
            moveMouse(dx, dy);
        }
    }

    else if (cmd.rfind("SCROLL:", 0) == 0) {
        int amount = 0;

        if (sscanf(cmd.c_str(), "SCROLL:%d", &amount) == 1) {
            amount = max(-1200, min(1200, amount));
            scrollMouse(amount);
        }
    }

    else
        return;

    std::cout << "Input: " << cmd << "\n";
}


void discoveryServer()
{
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sock == INVALID_SOCKET) {
        std::cout << "[DISCOVERY] Socket creation failed: "
                  << WSAGetLastError() << "\n";
        return;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DISCOVERY_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cout << "[DISCOVERY] Bind failed: "
                  << WSAGetLastError() << "\n";

        closesocket(sock);
        return;
    }

    std::cout << "[DISCOVERY] Listening on UDP "
              << DISCOVERY_PORT << "\n";

    const char response[] = "REMOTE_HOST_READY|1|45678";
    char buffer[256];

    while (true)
    {
        sockaddr_in client = {};
        int clientLen = sizeof(client);

        int received = recvfrom(
            sock,
            buffer,
            sizeof(buffer) - 1,
            0,
            (sockaddr*)&client,
            &clientLen
        );

        if (received == SOCKET_ERROR) {
            std::cout << "[DISCOVERY] Receive error: "
                      << WSAGetLastError() << "\n";
            continue;
        }

        buffer[received] = '\0';

        if (std::string(buffer) != "DISCOVER_REMOTE")
            continue;

        std::cout << "[DISCOVERY] Request from "
                  << inet_ntoa(client.sin_addr)
                  << ":" << ntohs(client.sin_port) << "\n";

        int sent = sendto(
            sock,
            response,
            sizeof(response) - 1,
            0,
            (sockaddr*)&client,
            clientLen
        );

        if (sent == SOCKET_ERROR) {
            std::cout << "[DISCOVERY] SEND FAILED: "
                      << WSAGetLastError() << "\n";
        }
        else if (sent != sizeof(response) - 1) {
            std::cout << "[DISCOVERY] Incomplete response\n";
        }
        else {
            std::cout << "[DISCOVERY] Response delivered to "
                      << inet_ntoa(client.sin_addr)
                      << ":" << ntohs(client.sin_port)
                      << " (" << sent << " bytes)\n";
        }
    }

    closesocket(sock);
}


int main()
{
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    std::thread(discoveryServer).detach();

    SOCKET server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(COMMAND_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (server == INVALID_SOCKET ||
        bind(server, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cout << "Command server failed. Check port.\n";
        if (server != INVALID_SOCKET) closesocket(server);
        WSACleanup();
        return 1;
    }

    std::cout << "\nUniversal Presentation Remote\n";
    std::cout << "Command port: " << COMMAND_PORT << "\n";
    std::cout << "Discovery port: " << DISCOVERY_PORT << "\n";
    std::cout << "Ready!\n\n";

    char buffer[128];

    while (true)
    {
        int n = recvfrom(
            server, buffer, sizeof(buffer) - 1,
            0, nullptr, nullptr
        );

        if (n <= 0)
            continue;

        buffer[n] = '\0';
        executeCommand(std::string(buffer));
    }

    closesocket(server);
    WSACleanup();

    return 0;
}
