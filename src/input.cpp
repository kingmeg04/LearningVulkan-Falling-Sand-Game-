#include "../include/input.h"

Input::Input(GLFWwindow* window)
    : window(window)
{
    glfwSetWindowUserPointer(window, this);
    glfwSetScrollCallback(window, scrollCallback);
}

void Input::update()
{
    previousMousePos = mousePos;

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    mousePos = glm::vec2(x, y);
    mouseDeltaValue = mousePos - previousMousePos;
}

bool Input::keyDown(int key) const
{
    return glfwGetKey(window, key) == GLFW_PRESS;
}

bool Input::mouseButtonDown(int button) const
{
    return glfwGetMouseButton(window, button) == GLFW_PRESS;
}

glm::vec2 Input::mousePosition() const
{
    return mousePos;
}

glm::vec2 Input::mouseDelta() const
{
    return mouseDeltaValue;
}

void Input::scrollCallback(
    GLFWwindow* window,
    double xOffset,
    double yOffset)
{
    auto* input =
        static_cast<Input*>(glfwGetWindowUserPointer(window));

    input->scrollX_ += xOffset;
    input->scrollY_ += yOffset;
}

