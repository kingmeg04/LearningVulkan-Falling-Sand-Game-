#ifndef INPUT_H
#define INPUT_H

#pragma once

#include "pch.h"

class Input
{
public:

    short scrollX_ = 0;
    short scrollY_ = 0;

    explicit Input(GLFWwindow* window);

    void update();

    float getDelta() const;

    bool keyDown(int key) const;

    bool mouseButtonDown(int button) const;

    glm::vec2 mousePosition() const;
    glm::vec2 mouseDelta() const;

private:
    std::chrono::time_point<std::chrono::system_clock> prevTime;
    std::chrono::time_point<std::chrono::system_clock> curTime;

    GLFWwindow* window;

    glm::vec2 mousePos{0.0f};
    glm::vec2 previousMousePos{0.0f};
    glm::vec2 mouseDeltaValue{0.0f, 0.0f};

    static void scrollCallback(
        GLFWwindow* window,
        double xOffset,
        double yOffset
    );
};



#endif //INPUT_H
