//
// SMD loader
//

#include <fstream>
#include <vector>

#include <cstring>
#include <cassert>

#include "smdloader.hpp"

namespace rendersystem
{
	static inline bool IsWhitespace(char ch)
	{
		return (ch == ' ' || ch == '\t');
	}

	void SMDLoader::ParseSMD(std::ifstream& handle)
	{
		constexpr const char szTriangles[]	= "triangles";
		constexpr const char szEnd[]		= "end";

		char buf[512];
		bool inTriangles = false;
		State state = State::Material;
		while (handle.good())
		{
			handle.getline(buf, sizeof(buf));
			char* bufPtr = buf;

			assert(bufPtr);

			if (inTriangles)
			{
				if (state == State::Material)
				{
					// Skip
					state = State::Vert1;
					continue;
				}
				else if (state == State::Vert1 || state == State::Vert2 || state == State::Vert3)
				{
					// Skip bone id

					while (IsWhitespace(*bufPtr))
					{
						++bufPtr;
					}

					while (!IsWhitespace(*bufPtr))
					{
						++bufPtr;
					}

					// We should have the whitespace before the X Y Z vars

					float8 value;

					// Execute five times (X Y Z U V)
					for (int i = 0; i != 3; ++i)
					{
						char* end;
						value.arr[i] = strtof(bufPtr, &end);
						bufPtr = end;
					}

					m_vTris.push_back(value);

					switch (state)
					{
					case State::Vert1:
						state = State::Vert2;
						break;
					case State::Vert2:
						state = State::Vert3;
						break;
					case State::Vert3:
						state = State::Material;
						continue;
					}
				}
			}

			if (strncmp(buf, szTriangles, sizeof(szTriangles)) == 0)
			{
				// We found the beginning of a triangles block
				inTriangles = true;
			}
			else if (inTriangles && (strncmp(buf, szEnd, sizeof(szEnd)) == 0))
			{
				return;
			}
		}
	}

	SMDLoader::SMDLoader(const wchar_t* filename)
	{
		std::ifstream handle(filename);
		if (handle.is_open() && handle.good()) {
			ParseSMD(handle);
		}
	}

}
