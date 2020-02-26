#include <windows.h>
#include <xnamath.h>
#include <iostream>
#include <d3dApp.h>
#include <d3dx11effect.h>
#include <MathHelper.h>
#include <regex>
#include <string>

using namespace std;

string ExePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    string::size_type pos = string( buffer ).find_last_of( "\\/" );
    return string(buffer).substr( 0, pos);
}

wstring s2ws(const std::string& s) {
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    wstring r(buf);
    delete[] buf;
    return r;
}

std::vector<std::string> splitR(const std::string& input, const std::string& regex) {
    std::regex re(regex);
    std::sregex_token_iterator first(input.begin(), input.end(), re, -1);
	std::sregex_token_iterator last;
    return std::vector<std::string>(first, last);
}

inline void split(const std::string &in, std::vector<std::string> &out, std::string token) {
	out.clear();
	std::string temp;
	int sz = int(in.size());
	for (int i = 0; i < sz; ++i) {
		std::string test = in.substr(i, token.size());
		if (test == token) {
			if (!temp.empty()) {
				out.push_back(temp);
				temp.clear();
				i += (int)token.size() - 1;
			}
			else {
				out.push_back("");
			}
		}
		else if (i + token.size() >= in.size()) {
			temp += in.substr(i, token.size());
			out.push_back(temp);
			break;
		}
		else {
			temp += in[i];
		}
	}
}

class ObjLoader {
public:

	struct Vertex {
		XMFLOAT3 Pos;
		XMFLOAT4 Color;
	};

	struct Material {
		std::string Name;
		XMFLOAT3 AmbientColor;
		XMFLOAT3 DiffuseColor;
		XMFLOAT3 SpecularColor;
		float SpecularCoefficient;
        float OpticalDensity;
		float Transparency;
        int IlluminationModel;
        std::string AmbientTextureMap;
		std::string DiffuseTextureMap;
        std::string SpecularTextureMap;
        std::string SpecularHighlightTextureMap;
        std::string BumpMap;
        std::string DisplacementMap;
        std::string StencilDecalMap;
        std::string AlphaTextureMap;
	};

	struct FaceVertex {
		int VertexIndex;
        int TextureIndex;
        int NormalIndex;
		//V = 0, VT = 1, VN = 2, VTN = 3
		int FaceType;
	};

	struct Face {
		std::vector<FaceVertex> vertices;
	};

	struct Group {
		std::string Name;
		Material Material;
		std::vector<Face> Faces;
	};

	struct ObjectContainer {
		std::string Name;
		Material Material;
		std::vector<Face> Faces;
		std::vector<Group> Groups;
	};

	ObjLoader(std::string fileName) {
		LoadObjFile(fileName);
	}

	std::vector<Vertex> Vertices;
	std::vector<XMFLOAT2> Textures;
	std::vector<XMFLOAT3> Normals;
	std::vector<Face> Faces;
	std::vector<int> Indices;
	std::vector<Group> Groups;
	std::vector<ObjectContainer> Objects;
	std::vector<Material> Materials;

private:
	void LoadObjFile(std::string fileName) {
		std::ifstream in(fileName);
		std::string line;
		if(in.is_open()) {

			Group currentGroup;
			Material currentMaterial;
			std::vector<Face> currentFaces;
			std::vector<std::string> tokens;
			bool firstGroup = true;
			while(std::getline(in, line)) {
				
				split(line, tokens, " ");
				int tokensCount = tokens.size();
				std::string keyword = tokens[0];
				
				if(keyword == "v" && tokensCount > 1) {
					float x = std::stof(tokens[1]);
					float y = std::stof(tokens[2]);
					float z = std::stof(tokens[3]);
					if(tokensCount > 4) {
						float r = std::stof(tokens[4]);
						float g = std::stof(tokens[5]);
						float b = std::stof(tokens[6]);
						Vertex vpos = {
							XMFLOAT3(x, y, z),
							XMFLOAT4(r, g, b, 1.0f),
						};
						Vertices.push_back(vpos);
					}
					else {
						Vertex vpos = {
							XMFLOAT3(x, y, z),
							XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),
						};
						Vertices.push_back(vpos);
					}
					continue;
				}

				if(keyword == "vt" && tokensCount > 1) {
					float u = std::stof(tokens[1]);
					float v = std::stof(tokens[2]);
					XMFLOAT2 vtex = XMFLOAT2(u, v);
					Textures.push_back(vtex);
					continue;
				}

				if(keyword == "vn" && tokensCount > 1) {
					float x = std::stof(tokens[1]);
					float y = std::stof(tokens[2]);
					float z = std::stof(tokens[3]);
					XMFLOAT3 vnor = XMFLOAT3(x, y, z);
					Normals.push_back(vnor);
					continue;
				}
				
				if(keyword == "f" && tokensCount > 1) {
					Face vface;
					std::vector<std::string> sindices;
					for(auto it = tokens.begin() + 1; it != tokens.end(); ++it) {
						FaceVertex vfaceInd;
						split(*it, sindices, "/");
						std::string sIP = sindices[0];
						std::string sIT;
						std::string sIN;
						if(sindices.size() > 1) {
							sIT = sindices[1];
							if(sindices.size() > 2) {
								sIN = sindices[2];
							}
						}
						if(sIP != "") vfaceInd.VertexIndex = std::stoi(sIP);
						if(sIT != "") vfaceInd.TextureIndex = std::stoi(sIT); else vfaceInd.TextureIndex = 0;
						if(sIN != "") vfaceInd.NormalIndex = std::stoi(sIN); else vfaceInd.NormalIndex = 0;
						//V = 0, VT = 1, VN = 2, VTN = 3
						if(sIP != "" && sIT != "" && sIN != "") {
							vfaceInd.FaceType = 3;
						}
						else if(sIP != "" && sIT != "" && sIN == "") {
							vfaceInd.FaceType = 1;
						}
						else if(sIP != "" && sIT == "" && sIN != "") {
							vfaceInd.FaceType = 2;
						}
						else if(sIP != "" && sIT == "" && sIN == "") {
							vfaceInd.FaceType = 0;
						}
						vface.vertices.push_back(vfaceInd);
						Indices.push_back(vfaceInd.VertexIndex - 1);
					}
					Faces.push_back(vface);
					currentFaces.push_back(vface);
					continue;
				}

				if(keyword == "mtllib" && tokensCount > 1) {
					std::string mtlFilePath = tokens[1];
					LoadMtlFile(mtlFilePath);
					continue;
				}

				if(keyword == "usemtl" && tokensCount > 1) {
					std::string mtlName = tokens[1];
					for(int i = 0; i < Materials.size(); ++i){
						if(Materials[i].Name == mtlName){
							currentMaterial = Materials[i];
						}
						break;
					}
					continue;
				}
				
				if(keyword == "g" && tokensCount > 1) {
					std::string groupName = tokens[1];
					currentGroup.Name = groupName;
					if(!firstGroup) {
						copy(currentFaces.begin(), currentFaces.end(), std::back_inserter(currentGroup.Faces));
						currentFaces.clear();
						currentGroup.Material = currentMaterial;
						Groups.push_back(currentGroup);
						currentGroup = Group();
					}
					firstGroup = false;
					continue;
				}
			}
			copy(currentFaces.begin(), currentFaces.end(), std::back_inserter(currentGroup.Faces));
			currentGroup.Material = currentMaterial;
			Groups.push_back(currentGroup);
		}
		in.close();
	}

	void LoadMtlFile(std::string fileName) {

		std::ifstream in(fileName);
		std::string line;
		if(in.is_open()) {
			Material newMaterial;
			std::vector<std::string> tokens;
			while(std::getline(in, line)) {
				
				split(line, tokens, " ");
				int tokensCount = tokens.size();
				std::string keyword = tokens[0];
				
				if(keyword == "newmtl" && tokensCount > 1) {
					newMaterial.Name = tokens[1];
					continue;
				}
				// Ambient Color
				if(keyword == "Ka" && tokensCount > 1) {
					float x = std::stof(tokens[1]);
					float y = std::stof(tokens[2]);
					float z = std::stof(tokens[3]);
					newMaterial.AmbientColor = XMFLOAT3(x, y, z);
					continue;
				}
				// Diffuse Color
				if(keyword == "Kd" && tokensCount > 1) {
					float x = std::stof(tokens[1]);
					float y = std::stof(tokens[2]);
					float z = std::stof(tokens[3]);
					newMaterial.DiffuseColor = XMFLOAT3(x, y, z);
					continue;
				}
				// Specular Color
				if(keyword == "Ks" && tokensCount > 1) {
					float x = std::stof(tokens[1]);
					float y = std::stof(tokens[2]);
					float z = std::stof(tokens[3]);
					newMaterial.SpecularColor = XMFLOAT3(x, y, z);
					continue;
				}
				// Specular Exponent
				if(keyword == "Ns" && tokensCount > 1) {
					float ns = std::stof(tokens[1]);
					newMaterial.SpecularCoefficient = ns;
					continue;
				}
				// Optical Density
				if(keyword == "Ni" && tokensCount > 1) {
					float ni = std::stof(tokens[1]);
					newMaterial.OpticalDensity = ni;
					continue;
				}
				// Dissolve transparency
				if(keyword == "d" && tokensCount > 1) {
					float d = std::stof(tokens[1]);
					newMaterial.Transparency = d;
					continue;
				}
				// Illumination model
				if(keyword == "illum" && tokensCount > 1) {
					int i = std::stoi(tokens[1]);
					newMaterial.IlluminationModel = i;
					continue;
				}
				// Ambient Texture Map
				if(keyword == "map_Ka" && tokensCount > 1) {
					newMaterial.AmbientTextureMap = tokens[1];
					continue;
				}
				// Diffuse Texture Map
				if(keyword == "map_Kd" && tokensCount > 1) {
					newMaterial.DiffuseTextureMap = tokens[1];
					continue;
				}
				// Specular Texture Map
				if(keyword == "map_Ks" && tokensCount > 1) {
					newMaterial.SpecularTextureMap = tokens[1];
					continue;
				}
				// Specular Hightlight Map
				if(keyword == "map_Ns" && tokensCount > 1) {
					newMaterial.SpecularHighlightTextureMap = tokens[1];
					continue;
				}
				// Alpha Texture Map
				if(keyword == "map_d" && tokensCount > 1) {
					newMaterial.AlphaTextureMap = tokens[1];
					continue;
				}
				// Bump Map
				if(keyword == "map_Bump" && tokensCount > 1) {
					newMaterial.BumpMap = tokens[1];
					continue;
				}
			}
			Materials.push_back(newMaterial);
		}
		in.close();
	}
};

ostream& operator<<(ostream& os, FXMVECTOR v) {

	XMFLOAT4 dest;
	XMStoreFloat4(&dest, v);

	os << "(" << dest.x << ", " << dest.y << ", " << dest.z << ", " << dest.w << ")";

	return os;
}

ostream& operator<<(ostream& os, CXMMATRIX m) {

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			os << m(i, j) << "\t";
		}
		os << endl;
	}

	return os;
}

struct Vertex {
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

class BoxApp : public D3DApp {

public:
	BoxApp(HINSTANCE hInstance);
	~BoxApp();

	bool Init();

	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void BuildGeometryBuffers();
	void BuildFX();
	void BuildVertexLayout();

	ID3D11Buffer* mBoxVB;
	ID3D11Buffer* mBoxIB;

	ID3DX11Effect* mFX;
	ID3DX11EffectTechnique* mTech;
	ID3DX11EffectMatrixVariable* mfxWorldViewProj;

	ID3D11InputLayout* mInputLayout;

	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	int _frame;
	float _tt;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {

#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	if(!XMVerifyCPUSupport()) {
		cout << "xna math not supported" << endl;
		return 0;
	}

	BoxApp theApp(hInstance);

	if(!theApp.Init()) {
		return 0;
	}

	return theApp.Run();
}

BoxApp::BoxApp(HINSTANCE hInstance) : D3DApp(hInstance), mBoxVB(0), mBoxIB(0), mFX(0), mTech(0), mfxWorldViewProj(0), mInputLayout(0), mTheta(1.5f * MathHelper::Pi), mPhi(0.25f * MathHelper::Pi), mRadius(5.0f) {
	mMainWndCaption = L"Box Demo";

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	_frame = 0;
	_tt = 0.0f;

	XMMATRIX W = XMMatrixScaling(0.004f, 0.004f, 0.004f);
	XMMATRIX I = XMMatrixIdentity();
	//XMStoreFloat4x4(&mWorld, I);
	XMStoreFloat4x4(&mWorld, W);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);
}

BoxApp::~BoxApp() {
	ReleaseCOM(mBoxVB);
	ReleaseCOM(mBoxIB);
	ReleaseCOM(mFX);
	ReleaseCOM(mInputLayout);
}

bool BoxApp::Init() {
	if(!D3DApp::Init()) {
		return false;
	}
	BuildGeometryBuffers();
	BuildFX();
	BuildVertexLayout();

	return true;
}

void BoxApp::OnResize() {
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void BoxApp::UpdateScene(float dt) {

	_tt += mTimer.DeltaTime();

	if(_tt > (1.0f / 24.0f)) {
		_tt = 0.0f;
		_frame += 1;
		if(_frame == 19) {
			_frame = 0;
		}
	}

	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);
}

void BoxApp::DrawScene() {
	assert(md3dImmediateContext);
	assert(mSwapChain);

	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Grey));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(mInputLayout);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;

	mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));

	D3DX11_TECHNIQUE_DESC techDesc;
	mTech->GetDesc(&techDesc);
	for(UINT p = 0; p < techDesc.Passes; ++p) {
		mTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
		//md3dImmediateContext->DrawIndexed(36, 0, 0);
		md3dImmediateContext->DrawIndexed(530 * 3, 530 * 3 * _frame, 0);
		//md3dImmediateContext->DrawIndexed(12 * 3, 0, 0);
		//md3dImmediateContext->DrawIndexed(1 * 3, 0, 0);
	}

	HR(mSwapChain->Present(0, 0));
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y) {
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y) {
	ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y) {
	if((btnState & MK_LBUTTON) != 0) {
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mTheta += dx;
		mPhi += dy;

		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if((btnState & MK_RBUTTON) != 0) {
		float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void BoxApp::BuildGeometryBuffers() {

	ObjLoader loader = ObjLoader(std::string("sp2_648.obj"));
	//ObjLoader loader = ObjLoader(std::string("simpleBox5.obj"));

	/*
	Vertex vertices[] = {
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), (const float*)&Colors::White   },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), (const float*)&Colors::Black   },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), (const float*)&Colors::Red     },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), (const float*)&Colors::Green   },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), (const float*)&Colors::Blue    },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), (const float*)&Colors::Yellow  },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), (const float*)&Colors::Cyan    },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), (const float*)&Colors::Magenta }
	};*/

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	//vbd.ByteWidth = sizeof(Vertex) * 8;
	vbd.ByteWidth = sizeof(ObjLoader::Vertex) * loader.Vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	
	D3D11_SUBRESOURCE_DATA vinitData;
	//vinitData.pSysMem = vertices;
	vinitData.pSysMem = loader.Vertices.data();

	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));

	/*
	UINT indices[] = {
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3, 
		4, 3, 7
	};*/

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	//ibd.ByteWidth = sizeof(UINT) * 36;
	int ffg = loader.Groups[0].Faces.size();
	ibd.ByteWidth = sizeof(UINT) * loader.Groups[0].Faces.size() * 3 * 19;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	//iinitData.pSysMem = indices;
	iinitData.pSysMem = loader.Indices.data();

	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
}

void BoxApp::BuildFX() {
	DWORD shaderFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	shaderFlags |= D3D10_SHADER_DEBUG;
	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

	ID3D10Blob* compiledShader = 0;
	ID3D10Blob* compilationMsgs = 0;

	std::string path = ExePath() + std::string("\\color.fx");
	std::string wpath = std::regex_replace(path, std::regex("\\\\"), std::string("/"));

	//HRESULT hr = D3DX11CompileFromFile(L"c:\color.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);
	HRESULT hr = D3DX11CompileFromFile(s2ws(wpath).c_str(), 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

	if(compilationMsgs != 0) {
		MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
		ReleaseCOM(compilationMsgs);
	}

	if(FAILED(hr)) {
		DXTrace(__FILE__, (DWORD)__LINE__, hr, L"D3DX11CompileFromFile", true);
	}

	HR(D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, md3dDevice, &mFX));
	ReleaseCOM(compiledShader);

	mTech = mFX->GetTechniqueByName("ColorTech");
	mfxWorldViewProj = mFX->GetVariableByName("gWorldViewProj")->AsMatrix();
}

void BoxApp::BuildVertexLayout() {
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	D3DX11_PASS_DESC passDesc;
	mTech->GetPassByIndex(0)->GetDesc(&passDesc);
	HR(md3dDevice->CreateInputLayout(vertexDesc, 2, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &mInputLayout));
}