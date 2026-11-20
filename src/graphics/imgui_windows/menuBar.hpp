#pragma once

#include <GL/glew.h>

#include "about.hpp"
#include "imguiWindow.hpp"
#include "controller.hpp"
#include "infoExchange.hpp"

class menuBar : public imguiWindow{
private:
    infoExchange* channel; // we need shared data (paths)
    bool m_pendingExportModal = false;  // just a helper flag for opening popup windows
public:
    menuBar(infoExchange* c);
    void draw() override;
};