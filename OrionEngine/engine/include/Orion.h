/// <summary>
/// This header acts as the access point for the subsystem API facades.
/// <summary>
#pragma once
#include <iostream>         // input/output
#include <fstream>          // file stream
#include <sstream>          // stream files into strings
#include <string> 
#include <vector>

#include "Application.h"

#include "Layers/Layer.h"
#include "Layers/ImGuiLayer.h"
#include "Layers/EditorLayer.h"
#include "Layers/RuntimeLayer.h"

#include "Log/Log.h"

#include "Scripting/Apollo.h"

#include "Renderer/Renderer.h"