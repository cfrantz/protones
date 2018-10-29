#ifndef IMWIDGET_PYTHON_CONSOLE_H
#define IMWIDGET_PYTHON_CONSOLE_H
#include <string>
#include <map>
#include <functional>
#include <vector>
#include "imwidget/imwidget.h"
#include "imgui.h"
#include "pybind11/embed.h"

class PythonConsole: public ImWindowBase {
  public:
    PythonConsole(pybind11::object hook, const char* name);
    PythonConsole(pybind11::object hook)
      : PythonConsole(hook, "PythonConsole") {}
    ~PythonConsole() override;

    void ClearLog();
    void AddLog(const char* fmt, ...); // IM_PRINTFARGS(2);
    bool Draw() override;
    void ExecCommand(const char* command_line);
  private:
    int TextEditCallback(ImGuiTextEditCallbackData* data);

    static int TextEditCallbackStub(ImGuiTextEditCallbackData* data);

    const char* name_;
    char inputbuf_[256];
    ImVector<char*> items_;
    bool scroll_to_bottom_;
    ImVector<char*> history_;
    // -1: new line, 0..history_.Size-1 browsing history.
    int history_pos_;
    bool more_;
    std::string source_;
    pybind11::object hook_;
};

#endif // IMWIDGET_PYTHON_CONSOLE_H
