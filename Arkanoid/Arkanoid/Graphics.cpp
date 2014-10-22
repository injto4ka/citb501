#include "graphics.h"

#pragma warning(disable:4996)

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

void CreateLight(GLenum id, GLfloat *ambient, GLfloat *diffuse, GLfloat *position)
{
	glLightfv(id, GL_AMBIENT, ambient);				// Setup The Ambient Light
	glLightfv(id, GL_DIFFUSE, diffuse);				// Setup The Diffuse Light
	glLightfv(id, GL_POSITION, position);			// Position The Light
	glEnable(id);                                   // Enable Light One
}

void CreateFog(GLfloat start, GLfloat end, GLfloat *color, GLfloat density, GLuint func, GLenum quality)
{
	glFogi(GL_FOG_MODE, func);
	glFogf(GL_FOG_DENSITY, density);
	glFogf(GL_FOG_START, start);
	glFogf(GL_FOG_END, end); 
	glFogfv(GL_FOG_COLOR, color);
	glHint(GL_FOG_HINT, quality);
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
	if(m_uComps<PIXEL_COMP_RGB)
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
	TGAHeader header;
	if(	!fp || !fread(&header, sizeof(TGAHeader), 1, fp) )
		return "Failed to read the image header!";
	if( header.m_iImageTypeCode != 2 || !SetSize(header.m_iWidth, header.m_iHeight, header.m_iBPP / 8))
		return "Unsupported image format!";
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

void Image::Draw(float x, float y)
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
			childs,			// number of color childs
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
			childs,			// number of color childs
			width,			// width
			height,			// height
			border,			// border
			format,			// color model
			GL_UNSIGNED_BYTE, // data word type
			data);			// data
	}
	//GL_INVALID_VALUE

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

GLvoid DrawSphere(float R, int nDivs)
{
	if(R < 0 || nDivs <= 0)
		return;
	int nLat = nDivs;
	int nLon = 2 * nDivs;
	float
		u = 0, v = 0, du = 1.0f/nLon, dv = 1.0f/nLat, 
		N[3], 
		lat = -0.5*PI, lon = 0, 
		dlat = PI/nLat, dlon = 2*PI/nLon, 
		clat, slat, 
		clat0 = 0, slat0 = -1, 
		clon, slon, 
		clon0 = 1, slon0 = 0;

	glBegin(GL_QUADS); // Start Drawing Quads
	for(int i = 0;i < nLat;i++)
	{
		// lat update
		lat += dlat;
		clat = cosf(lat);
		slat = sinf(lat);
		
		u = 0;
		for(int j = 0;j < nLon;j++)
		{
			// lon update
			lon += dlon;
			clon = cosf(lon);
			slon = sinf(lon);
			
			///////////////////////////////////////////
			// 
			//		(u, v+dv)	(u+du, v+dv)
			//		P3----------P2	lon
			//		|			|
			//		|			|	/\
			//		|			|
			//		P0----------P1	lon0
			//		(u, v)		(u+du, v)	
			//		lat0	>	lat
			//
			///////////////////////////////////////////

			// P0: lat = lat0, lon = lon0
			N[0] = clon0*clat0;
			N[1] = slon0*clat0;
			N[2] = slat0;
			glNormal3d(N[0], N[1], N[2]);
			glTexCoord2d(u, v);
			glVertex3d(N[0]*R, N[1]*R, N[2]*R);
			
			// P1: lat = lat0, lon = lon
			N[0] = clon*clat0;
			N[1] = slon*clat0;
			N[2] = slat0;
			glNormal3d(N[0], N[1], N[2]);
			glTexCoord2d(u+du, v);
			glVertex3d(N[0]*R, N[1]*R, N[2]*R);

			// P2: lat = lat, lon = lon
			N[0] = clon*clat;
			N[1] = slon*clat;
			N[2] = slat;
			glNormal3d(N[0], N[1], N[2]);
			glTexCoord2d(u+du, v+dv);
			glVertex3d(N[0]*R, N[1]*R, N[2]*R);

			// P3: lat = lat, lon = lon0
			N[0] = clon0*clat;
			N[1] = slon0*clat;
			N[2] = slat;
			glNormal3d(N[0], N[1], N[2]);
			glTexCoord2d(u, v+dv);
			glVertex3d(N[0]*R, N[1]*R, N[2]*R);
			
			clon0 = clon;
			slon0 = slon;
			u += du;
		}
		clat0 = clat;
		slat0 = slat;
		v += dv;
	}
	glEnd();
}