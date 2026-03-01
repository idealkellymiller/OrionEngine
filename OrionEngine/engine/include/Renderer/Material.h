#pragma once


class Material {
public:
	Material() = default;
	Material(const Material& other) = default;
	Material(Material&& other) noexcept = default;
	Material& operator=(const Material& other) = default;
	Material& operator=(Material&& other) noexcept = default;
	virtual ~Material() = default;
};