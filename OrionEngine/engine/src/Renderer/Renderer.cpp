#include <iostream>

#include "Renderer/Renderer.h"


bool Renderer::Init() {
	printf("Initializing Renderer...\n");
	
	printf("Renderer initialized successfully.\n");
	printf("Testttttting.\n");
	printf("Test stuffzzzz.\n");
	return true;
}

void Renderer::Shutdown() {
	printf("Renderer shutdown successfully.\n");
}

void Renderer::beginFrame(float dt) {

}

void Renderer::RenderFrame() {

}

void Renderer::EndFrame() {

}

void Renderer::SetScene(Scene* scene) {

}

void Renderer::SyncScene() {

}

void Renderer::SetViewportSize(int w, int h) {

}

TextureHandle Renderer::GetViewportTexture() {
	TextureHandle tex;
	return tex;
}

void Renderer::RebuildPassList() {

}

void Renderer::ExecutePasses() {

}

int Renderer::PickObject() {
	return -1;
}

