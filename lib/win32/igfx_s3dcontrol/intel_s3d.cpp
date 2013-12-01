#ifdef _WIN32

#include <windows.h>
#include <comdef.h>
#include <igfx_s3dcontrol.h>

#define DLLEXPORT __declspec(dllexport)

extern "C"
{
  DLLEXPORT IGFXS3DControl * CreateIGFXS3DControlEx(void)
  {
    return CreateIGFXS3DControl();
  }
}

#endif