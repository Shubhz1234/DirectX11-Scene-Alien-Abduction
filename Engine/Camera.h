#pragma once
class Camera
{
public:
	Camera();
	~Camera();

	void							Update();
	DirectX::SimpleMath::Matrix		getCameraMatrix();
	void							setPosition(DirectX::SimpleMath::Vector3 newPosition);
	DirectX::SimpleMath::Vector3	getPosition();
	DirectX::SimpleMath::Vector3	getForward();
	DirectX::SimpleMath::Vector3    getUpwards();
	void							setRotation(DirectX::SimpleMath::Vector3 newRotation);
	DirectX::SimpleMath::Vector3	getRotation();
	float							getMoveSpeed();
	float							getRotationSpeed();
	float                           getUpSpeed();

private:
	DirectX::SimpleMath::Matrix		m_cameraMatrix;			//camera matrix to be passed out and used to set camera position and angle for wrestling
	DirectX::SimpleMath::Vector3	m_lookat;
	DirectX::SimpleMath::Vector3	m_position;
	DirectX::SimpleMath::Vector3	m_forward;
	DirectX::SimpleMath::Vector3    m_upwards;
	DirectX::SimpleMath::Vector3	m_right;
	DirectX::SimpleMath::Vector3	m_orientation;			//vector storing pitch yaw and roll. 

	float	m_movespeed ;	
	float	m_camRotRate;
	float   m_upSpeed;
};

