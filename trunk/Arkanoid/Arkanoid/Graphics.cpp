#include "graphics.h"
#include <algorithm>

ErrorCode glErrorToStr()
{
	switch(glGetError())
	{
		case GL_NO_ERROR:			return NULL;
		case GL_INVALID_ENUM:		return "An unacceptable value is specified for an enumerated argument. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_INVALID_VALUE:		return "A numeric argument is out of range. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_INVALID_OPERATION:	return "The specified operation is not allowed in the current state. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_STACK_OVERFLOW:		return "This function would cause a stack overflow. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_STACK_UNDERFLOW:	return "This function would cause a stack underflow. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_OUT_OF_MEMORY:		return "There is not enough memory left to execute the function. The state of OpenGL is undefined, except for the state of the error flags, after this error is recorded.";
		default:					return "Unknown error.";
	}
}

void SetMemAlign(int nMemWidth, BOOL bPack)
{
	if(nMemWidth <= 0)
		return;
	for(GLint nAlign = 8;;nAlign >>= 1)
	{
		if(nAlign == 1 || nMemWidth % nAlign == 0)
		{
			glPixelStorei(bPack ? GL_PACK_ALIGNMENT : GL_UNPACK_ALIGNMENT, nAlign);
			return;
		}
	}
}

void CreateLight(GLenum id, GLfloat *position, GLfloat *ambient, GLfloat *diffuse, GLfloat *specular, GLfloat *attenuate)
{
	if( ambient )
		glLightfv(id, GL_AMBIENT, ambient);				// Setup The Ambient Light
	if( diffuse )
		glLightfv(id, GL_DIFFUSE, diffuse);				// Setup The Diffuse Light
	if( specular )
		glLightfv(id, GL_SPECULAR, specular);			// Position The Light
	if( position )
		glLightfv(id, GL_POSITION, position);			// Position The Light
	if( attenuate )
	{
		glLightf(id, GL_CONSTANT_ATTENUATION, attenuate[0]);
		glLightf(id, GL_LINEAR_ATTENUATION, attenuate[1]);
		glLightf(id, GL_QUADRATIC_ATTENUATION, attenuate[2]);
	}
	glEnable(id);                                   // Enable Light One
}

BOOL Image::SetSize(int nNewWidth, int nNewHeight, int nNewComp)
{
	if(nNewWidth <= 0 || nNewHeight <= 0 ||
		nNewComp != PIXEL_COMP_GRAY && nNewComp != PIXEL_COMP_RGB && nNewComp != PIXEL_COMP_RGBA)
		return FALSE;
	m_vBuffer.resize(nNewWidth * nNewHeight * nNewComp);
	m_uComps = nNewComp;
	m_uWidth = nNewWidth;
	m_uHeight = nNewHeight;
	return TRUE;
}

void Image::FlipV()
{
	char *pData = GetDataPtr();
	const int my = m_uHeight/2, B = m_uComps * m_uWidth, D = (m_uHeight - 1) * B;
	for(int i = 0 ; i < m_uWidth; i++)
	{
		const int A = m_uComps * i, C = A + D;
		for(int j = 0; j < my; j++)
		{
			const int p1 = A + j * B, p2 = C - j * B;
			switch(m_uComps)
			{
			case PIXEL_COMP_RGBA:
				REPLACE(pData[p1+3], pData[p2+3]);
			case PIXEL_COMP_RGB:
				REPLACE(pData[p1+2], pData[p2+2]);
				REPLACE(pData[p1+1], pData[p2+1]);
			case PIXEL_COMP_GRAY:
				REPLACE(pData[p1], pData[p2]);
			}
		}
	}
}

void Image::FlipC()
{
	if(m_uComps < PIXEL_COMP_RGB)
		return;
	const int m=m_uComps;
	const int s=m_uWidth*m_uHeight;
	const void *d=GetDataPtr();
	__asm							// Assembler Code To Follow
	{
		mov ecx, s					// Counter Set To Dimensions Of Our Memory Block
		mov ebx, d					// Points ebx To Our Data (b)
		label:						// Label Used For Looping
		mov al,[ebx+0]				// Loads Value At ebx Into al
		mov ah,[ebx+2]				// Loads Value At ebx+2 Into ah
		mov [ebx+2],al				// Stores Value In al At ebx+2
		mov [ebx+0],ah				// Stores Value In ah At ebx
		
		add ebx,m					// Moves Through The Data
		dec ecx						// Decreases Our Loop Counter
		jnz label					// If Not Zero Jump Back To Label
	}
}

// Read TGA format rgb or rgba image
ErrorCode Image::ReadTGA(FILE* fp)
{
	char pchIdentification[256] = "";
	TGAHeader header;
	if(	!fp || !fread(&header, sizeof(TGAHeader), 1, fp) )
		return "Failed to read the image header!";
	if( header.m_iImageTypeCode != 2 || header.m_iIdentificationFieldSize >= 256 || !SetSize(header.m_iWidth, header.m_iHeight, header.m_iBPP / 8))
		return "Unsupported image format!";
	if( header.m_iIdentificationFieldSize && (header.m_iIdentificationFieldSize >= 256 || !fread(pchIdentification, 1, header.m_iIdentificationFieldSize, fp)) )
		return "Failed to read the image identification!";
	if( !fread(GetDataPtr(), 1, GetDataSize(), fp) )
		return "Failed to read the image data!";
	if(GET_BIT(header.m_ImageDescriptorByte, 5))
		FlipV();
	// flip red and blue components
	FlipC();
	return NULL; 
}

ErrorCode Image::ReadTGA(const char *pchFileName)
{
	File file;
	if( !file.Open(pchFileName) )
		return "Failed to open the file!";
	return ReadTGA(file);
}

void Image::Draw(float x, float y) const
{
	if(!m_uWidth)
		return;
	SetMemAlign(m_uWidth, FALSE);
	glRasterPos2f(x, y);
	glDrawPixels(m_uWidth, m_uHeight, GetPixelFormat(), GL_UNSIGNED_BYTE, GetDataPtr());
}

ErrorCode Texture::Create(const Image &image)
{
	Destroy();
	if(!image.GetDataSize())
		return "No image data!";
	GLuint format = image.GetPixelFormat();
	GLint width = image.GetWidth();
	GLint height = image.GetHeight();
	const char *data = image.GetDataPtr();

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

	if(mipmapped)
	{
		gluBuild2DMipmaps(
			GL_TEXTURE_2D,	// 2D image
			components,	    // number of color components
			width,			// width
			height,			// height
			format,			// color model
			GL_UNSIGNED_BYTE, // data word type
			data);			// data
	}
	else
	{
		glTexImage2D(
			GL_TEXTURE_2D,	// 2D image
			levelDetail,	// detail level
			components,		// number of color components
			width,			// width
			height,			// height
			border,			// border
			format,			// color model
			GL_UNSIGNED_BYTE, // data word type
			data);			// data
	}

	return glErrorToStr();
}

void Texture::Destroy()
{
	if( id != -1 )
	{
		glDeleteTextures(1, &id);
		id = -1;
	}
}

ErrorCode Texture::Bind() const
{
	if( id == -1 )
		return "No texture generated!";
	glBindTexture(GL_TEXTURE_2D, id);
	return glErrorToStr();
}

GLvoid DrawLine3D(
	float x1, float y1, float z1, 
	float x2, float y2, float z2,
	int c1, int c2, float w)
{
	glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_HINT_BIT | GL_CURRENT_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glLineWidth(w);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glBegin(GL_LINES);
		if( c1 )
			SetColor(c1);
		glVertex3f( x1, y1, z1);
		if( c2 )
			SetColor(c2);
		glVertex3f( x2, y2, z2);
	glEnd();

	glPopAttrib();
}

GLvoid DrawLine2D(
	float x1, float y1,
	float x2, float y2,
	int c, float w)
{
	glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_HINT_BIT | GL_CURRENT_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glLineWidth(w);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	
	glBegin(GL_LINES);
		SetColor(c);
		glVertex2f( x1, y1);
		glVertex2f( x2, y2);
	glEnd();

	glPopAttrib();
}

GLvoid DrawFrame(float x, float y, float z, float r, float w)
{
	glPushAttrib(GL_TRANSFORM_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_HINT_BIT | GL_CURRENT_BIT);
	glPushMatrix();
	glTranslatef(x, y, z);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glLineWidth(w);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	
	glBegin(GL_LINES);
		// X in Red
		SetColor(0xff0000ff);
		glVertex3f( 0, 0, 0);
		glVertex3f( r, 0, 0);
		// Y in Green
		SetColor(0xff00ff00);
		glVertex3f( 0, 0, 0);
		glVertex3f( 0, r, 0);
		// Z in Blue
		SetColor(0xffff0000);
		glVertex3f( 0, 0, 0);
		glVertex3f( 0, 0, r);
	glEnd();

	glPopMatrix();
	glPopAttrib();
}

GLvoid DrawSphere(float R, int nDivs)
{
	if(R < 0 || nDivs <= 1)
		return;
	int nLat = 2 * nDivs;
	int nLon = 4 * nDivs;
	float
		u = 0, v = 0, v0 = 0, du = 1.0f/nLon, dv = 1.0f/nLat, 
		N[3], 
		lat = -0.5*PI, lon = 0, r = 0, r0 = 0, 
		dlat = PI/nLat, dlon = 2*PI/nLon,
		clat, slat, 
		clat0 = 0, slat0 = -1;
	bool bStart = true, bEnd = false;
	glBegin(GL_TRIANGLE_STRIP); // Start Drawing Quads
	for(int i = 0; i < nLat; i++)
	{
		// lat update
		if( i == nLat - 1 )
		{
			bEnd = true;
			r = 0;
			lat = 0.5*PI;
			clat = 0;
			slat = 1;
		}
		else
		{
			r = min(nLat - i - 1, i + 1) * dv;
			lat += dlat;
			clat = cosf(lat);
			slat = sinf(lat);
		}
		
		float u0 = 0, lon = 0, u = 0;

		N[0] = clat0;
		N[1] = 0;
		N[2] = slat0;
		glNormal3d(N[0], N[1], N[2]);
		glTexCoord2d(0.5f + r0, 0.5f);
		glVertex3d(N[0]*R, N[1]*R, N[2]*R);

		N[0] = clat;
		N[1] = 0;
		N[2] = slat;
		glNormal3d(N[0], N[1], N[2]);
		glTexCoord2d(0.5f + r, 0.5f);
		glVertex3d(N[0]*R, N[1]*R, N[2]*R);

		bool bEven = true;
		for(int j = 0; j < nLon; j++)
		{
			// lon update
			float clon, slon;
			if( j == nLon - 1 )
			{
				u = 1;
				lon = 2*PI;
				clon = 1;
				slon = 0;
			}
			else
			{
				u += du;
				lon += dlon;
				clon = cosf(lon);
				slon = sinf(lon);
			}
			
			///////////////////////////////////////////
			// 
			//		(u, v+dv)	(u+du, v+dv)
			//		P3----------P2	lat
			//		|			|
			//		|			|	/\
			//		|			|
			//		P0----------P1	lat0
			//		(u, v)		(u+du, v)	
			//		lon0	>	lon
			//
			///////////////////////////////////////////
			
			bEven = !bEven;
			if( !bStart || bEven )
			{
				N[0] = clon*clat0;
				N[1] = slon*clat0;
				N[2] = slat0;
				glNormal3d(N[0], N[1], N[2]);
				glTexCoord2d(0.5f + r0 * clon, 0.5f + r0 * slon);
				glVertex3d(N[0]*R, N[1]*R, N[2]*R);
			}

			if( !bEnd || bEven )
			{
				N[0] = clon*clat;
				N[1] = slon*clat;
				N[2] = slat;
				glNormal3d(N[0], N[1], N[2]);
				glTexCoord2d(0.5f + r * clon, 0.5f + r * slon);
				glVertex3d(N[0]*R, N[1]*R, N[2]*R);
			}
		}
		clat0 = clat;
		slat0 = slat;
		v0 = v;
		r0 = r;
		bStart = false;
	}
	glEnd();
}

GLvoid DrawCircle3D(
		float xc, float yc, float zc, float radius, int color,
		float xn, float yn, float zn,
		float line, int divs)
{
	glPushMatrix();
	glPushAttrib(GL_LINE_BIT | GL_CURRENT_BIT);
	if(color)
		SetColor(color);
	if(line)
		glLineWidth(line);
	glTranslatef(xc, yc, zc);
	glBegin(GL_LINE_STRIP);

	Quaternion q(Point(0, 0, 1), 360.0f / divs, true);
	glVertex3f( radius, 0, 0 );

	Point pt(radius, 0, 0);
	for(int i = 1; i < divs; i++)
	{
		pt = q * pt;
		glVertex3f( pt.x, pt.y, pt.z );
	}
	glVertex3f( radius, 0, 0 );

	glEnd();
	glPopAttrib();
	glPopMatrix();
}

GLvoid DrawSpline3D(
		const Point *coefs, int color,
		float line, int divs)
{
	glPushAttrib(GL_LINE_BIT | GL_CURRENT_BIT);
	if(color)
		SetColor(color);
	if(line)
		glLineWidth(line);
	glBegin(GL_LINE_STRIP);

	Point pt = SplinePos(coefs, 0);
	glVertex3f( pt.x, pt.y, pt.z );
	if( divs > 2 )
	{
		float k = 1.0f / (divs - 1);
		for(int i = 1; i < divs - 1; i++)
		{
			pt = SplinePos(coefs, i * k);
			glVertex3f( pt.x, pt.y, pt.z );
		}
	}
	pt = SplinePos(coefs, 1);
	glVertex3f( pt.x, pt.y, pt.z );

	glEnd();
	glPopAttrib();
}

void DrawBox(
		float left, float top,
		float width, float height,
		DWORD color,
		float line)
{
	glPushAttrib(GL_LINE_BIT | GL_CURRENT_BIT);
	if(color)
		SetColor(color);
	if(line)
		glLineWidth(line);
	glBegin(GL_LINE_STRIP);
		glVertex2f( left, top);
		glVertex2f( left, top+height);
		glVertex2f( left+width, top+height);
		glVertex2f( left+width, top);
		glVertex2f( left, top);
	glEnd();
	glPopAttrib();
}
void FillBox(
		float left, float top,
		float width, float height,
		DWORD color)
{
	glPushAttrib(GL_CURRENT_BIT);
	if(color)
		SetColor(color);
	glBegin(GL_QUADS);
		glVertex2f( left, top);
		glVertex2f( left, top+height);
		glVertex2f( left+width, top+height);
		glVertex2f( left+width, top);
	glEnd();
	glPopAttrib();
}

void DrawBox(
	int left, int top,
	int width, int height,
	DWORD color,
	float line)
{
	glPushAttrib(GL_LINE_BIT | GL_CURRENT_BIT);
	if (color)
		SetColor(color);
	if (line)
		glLineWidth(line);
	glBegin(GL_LINE_STRIP);
	glVertex2i(left, top);
	glVertex2i(left, top + height);
	glVertex2i(left + width, top + height);
	glVertex2i(left + width, top);
	glVertex2i(left, top);
	glEnd();
	glPopAttrib();
}
void FillBox(
	int left, int top,
	int width, int height,
	DWORD color)
{
	glPushAttrib(GL_CURRENT_BIT);
	if (color)
		SetColor(color);
	glBegin(GL_QUADS);
	glVertex2i(left, top);
	glVertex2i(left, top + height);
	glVertex2i(left + width, top + height);
	glVertex2i(left + width, top);
	glEnd();
	glPopAttrib();
}

void DrawCube(float side)
{
	const float r = 0.5f * side;

	glBegin(GL_QUADS);                          // Start Drawing Quads
	// Front Face
	glNormal3f(0.0f, 0.0f, 1.0f);                  // Normal Pointing Towards Viewer
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-r, -r, r);  // Point 1 (Front)
	glTexCoord2f(1.0f, 0.0f); glVertex3f(r, -r, r);  // Point 2 (Front)
	glTexCoord2f(1.0f, 1.0f); glVertex3f(r, r, r);  // Point 3 (Front)
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-r, r, r);  // Point 4 (Front)
	// Back Face
	glNormal3f(0.0f, 0.0f, -1.0f);                  // Normal Pointing Away From Viewer
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-r, -r, -r);  // Point 1 (Back)
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-r, r, -r);  // Point 2 (Back)
	glTexCoord2f(0.0f, 1.0f); glVertex3f(r, r, -r);  // Point 3 (Back)
	glTexCoord2f(0.0f, 0.0f); glVertex3f(r, -r, -r);  // Point 4 (Back)
	// Top Face
	glNormal3f(0.0f, 1.0f, 0.0f);                  // Normal Pointing Up
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-r, r, -r);  // Point 1 (Top)
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-r, r, r);  // Point 2 (Top)
	glTexCoord2f(1.0f, 0.0f); glVertex3f(r, r, r);  // Point 3 (Top)
	glTexCoord2f(1.0f, 1.0f); glVertex3f(r, r, -r);  // Point 4 (Top)
	// Bottom Face
	glNormal3f(0.0f, -1.0f, 0.0f);                  // Normal Pointing Down
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-r, -r, -r);  // Point 1 (Bottom)
	glTexCoord2f(0.0f, 1.0f); glVertex3f(r, -r, -r);  // Point 2 (Bottom)
	glTexCoord2f(0.0f, 0.0f); glVertex3f(r, -r, r);  // Point 3 (Bottom)
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-r, -r, r);  // Point 4 (Bottom)
	// Right face
	glNormal3f(1.0f, 0.0f, 0.0f);                  // Normal Pointing Right
	glTexCoord2f(1.0f, 0.0f); glVertex3f(r, -r, -r);  // Point 1 (Right)
	glTexCoord2f(1.0f, 1.0f); glVertex3f(r, r, -r);  // Point 2 (Right)
	glTexCoord2f(0.0f, 1.0f); glVertex3f(r, r, r);  // Point 3 (Right)
	glTexCoord2f(0.0f, 0.0f); glVertex3f(r, -r, r);  // Point 4 (Right)
	// Left Face
	glNormal3f(-1.0f, 0.0f, 0.0f);                  // Normal Pointing Left
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-r, -r, -r);  // Point 1 (Left)
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-r, -r, r);  // Point 2 (Left)
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-r, r, r);  // Point 3 (Left)
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-r, r, -r);  // Point 4 (Left)

	glEnd();
}

#ifdef _DEBUG
struct DbgObject
{
	virtual void Draw() const = 0;
	virtual int Type() const = 0;
	enum Types {
		DBG_VECTOR,
		DBG_CIRCLE,
		DBG_SPLINE,
		DBG_COUNT,
	};
	bool operator < (const DbgObject& other) const { return Type() < other.Type(); }
	static bool Compare(const DbgObject* a, const DbgObject* b) { return a->Type() < b->Type(); }
};
static std::vector<const DbgObject *> s_vDbgObjects;
static CriticalSection s_csDbgObjects;
static DisplayList s_dlDbg;
void DbgAdd(const DbgObject *obj)
{
	Lock lock(s_csDbgObjects);
	s_dlDbg.Destroy();
	s_vDbgObjects.push_back(obj);
	std::sort(s_vDbgObjects.begin(), s_vDbgObjects.end(), DbgObject::Compare);
}
void DbgDraw()
{
	if( !s_dlDbg )
	{
		Lock lock(s_csDbgObjects);
		if( !s_vDbgObjects.size() )
			return;
		CompileDisplayList cds(s_dlDbg);
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		for(size_t i = 0; i < s_vDbgObjects.size(); i++)
			s_vDbgObjects[i]->Draw();
		glPopAttrib();
	}
	s_dlDbg.Execute();
}
void DbgClear()
{
	Lock lock(s_csDbgObjects);
	for(size_t i = 0; i < s_vDbgObjects.size(); i++)
		delete s_vDbgObjects[i];
	s_vDbgObjects.clear();
	s_dlDbg.Destroy();
}

struct DbgVector: DbgObject
{
	float
		x1, y1, z1,
		x2, y2, z2,
		width;
	int color1, color2;
	DbgVector(
		float x1, float y1, float z1,
		float x2, float y2, float z2,
		int color1, int color2, float width):
			x1(x1), y1(y1), z1(z1),
			x2(x2), y2(y2), z2(z2),
			width(width), color1(color1), color2(color2)
	{}
	virtual void Draw() const
	{
		DrawLine3D(
			x1, y1, z1,
			x2, y2, z2,
			color1, color2, width);
	}
	virtual int Type() const
	{
		return DBG_VECTOR;
	}
};

void DbgAddVector(
	float x1, float y1, float z1,
	float x2, float y2, float z2,
	int color1, int color2, float width)
{
	DbgAdd(new DbgVector(
		x1, y1, z1,
		x2, y2, z2,
		color1, color2, width));
}

struct DbgCircle: DbgObject
{
	float
		xc, yc, zc, radius,
		xn, yn, zn,
		width;
	int color, divs;
	DbgCircle(
		float xc, float yc, float zc,
		float radius, int color,
		float xn, float yn, float zn,
		float width, int divs):
			xc(xc), yc(yc), zc(zc),
			radius(radius), color(color),
			xn(xn), yn(yn), zn(zn),
			width(width), divs(divs)
	{}
	virtual void Draw() const
	{
		DrawCircle3D(
			xc, yc, zc, radius, color,
			xn, yn, zn,
			width, divs);
	}
	virtual int Type() const
	{
		return DBG_CIRCLE;
	}
};

void DbgAddCircle(
	float xc, float yc, float zc,
	float radius, int color,
	float xn, float yn, float zn,
	float width, int divs)
{
	DbgAdd(new DbgCircle(
		xc, yc, zc, radius, color,
		xn, yn, zn,
		width, divs));
}

struct DbgSpline: DbgObject
{
	int steps, color;
	float width;
	Point coef[4];
	
	DbgSpline(
		float x1, float y1, float z1,
		float x2, float y2, float z2,
		float x3, float y3, float z3,
		float x4, float y4, float z4,
		int color, float width,
		float step, int steps):
			color(color), width(width), steps(steps)
	{
		Point pt[] = {
			Point(x1, y1, z1),
			Point(x2, y2, z2),
			Point(x3, y3, z3),
			Point(x4, y4, z4),
		};
		if( step )
		{
			float len = SplineLenEst(pt);
			steps = 1 + Trunc(len / step);
		}
		SplineCoefs(pt, coef);
	}
	virtual void Draw() const
	{
		DrawSpline3D(coef, color, width, steps);
	}
	virtual int Type() const
	{
		return DBG_SPLINE;
	}
};

void DbgAddSpline(
	float x1, float y1, float z1,
	float x2, float y2, float z2,
	float x3, float y3, float z3,
	float x4, float y4, float z4,
	int color, float width,
	float step, int steps)
{
	DbgAdd(new DbgSpline(
		x1, y1, z1,
		x2, y2, z2,
		x3, y3, z3,
		x4, y4, z4,
		color, width,
		step, steps));
}

#endif _DEBUG 