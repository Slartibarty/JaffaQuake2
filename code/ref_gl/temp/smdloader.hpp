//
// SMD loader
//

#pragma once

#include <fstream>
#include <vector>

struct float8
{
	float arr[3];
};

namespace rendersystem
{
	class SMDLoader
	{
	private:
		std::vector<float8> m_vTris;

	private:
		enum class State
		{
			Material,
			Vert1,
			Vert2,
			Vert3
		};

		void ParseSMD(std::ifstream& handle);

	public:
		SMDLoader(const wchar_t* filename);

		float8* GetTris(int& numTris) const
		{
			// Narrowing conversion: size_t to int
			numTris = static_cast<int>(m_vTris.size());
			// CASTS AWAY CONSTNESS!!!
			return (float8*)m_vTris.data();
		}

	};
}
