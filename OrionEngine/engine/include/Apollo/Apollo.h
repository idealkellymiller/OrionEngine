/// <summary>
/// Apollo is the ScriptEngine interface that user scripts will inherit to gain access to lifecycle hooks and functions
/// <summary>


#pragma once

class Apollo {
public:
	static void OnStart();
	static void OnUpdate();
	static void OnEnable();
	static void OnDisable();
};