#include "Orion.h"



// EXAMPLE OF HOW TO USE A LAYER IN EDITORCLIENT
class ExampleLayer : public Orion::Layer
{
public:
	ExampleLayer() : Layer("Example")
	{

	}

	void OnUpdate() override
	{
		// this is just to show how a layer updates every frame.
		// uncomment if you want to see this print message every frame
		//ORN_INFO("ExampleLayer::OnUpdate");
	}

	void OnEvent(Orion::Event& event) override
	{
        // on event, print the event using client logger
		// ORN_TRACE("{0}", event.ToString());
	}
};

class EditorClient : public Orion::Application
{
	public:
		EditorClient()
		{ 
			PushLayer(new ExampleLayer());
			PushLayer(new Orion::ImGuiLayer());
		}

		~EditorClient()
		{

		}
};

Orion::Application* Orion::CreateApplication() 
{
	return new EditorClient();
}

int main(int argc, char** argv)
{
	// initialize loggers
	Orion::Log::Init();

	ORN_CORE_WARN("Initalized Log!");
	ORN_INFO("Initalized Client Log!");

	// create app automatically
	printf("Orion Engine\n");
	auto app = Orion::CreateApplication();
	app->Run(); // starts main loop
	delete app;
}

// Luke:
// - Add namespace Orion {} to all .cpp/.h
// - Add #include "EngineCore.h" and the Orion_API to each class declaration
//