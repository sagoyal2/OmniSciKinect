#include "app.h"
#include "time.h"
#include <string>
#include <algorithm>
#include <vector>
#include <windows.h>

void App::Init()
{
  //put initialization stuff here

  HRESULT hr;
  SetCurrentDirectoryA("Z:\TEMP");
  myfile.open("allpoints.txt");

  //get the kinect sensor
  hr = GetDefaultKinectSensor(&m_sensor);
  if(FAILED(hr))
  {
    printf("Failed to find the kinect sensor!\n");
    exit(10);
  }
  m_sensor->Open();
  
  //get the depth frame source
  IDepthFrameSource* depthFrameSource;
  hr = m_sensor->get_DepthFrameSource(&depthFrameSource);
  if(FAILED(hr))
  {
    printf("Failed to get the depth frame source.\n");
    exit(10);
  }
  //get depth frame description
  IFrameDescription *frameDesc;
  depthFrameSource->get_FrameDescription(&frameDesc);
  frameDesc->get_Width(&m_depthWidth);
  frameDesc->get_Height(&m_depthHeight);

  //get the depth frame reader
  hr = depthFrameSource->OpenReader(&m_depthFrameReader);
  if(FAILED(hr))
  {
    printf("Failed to open the depth frame reader!\n");
    exit(10);
  }
  //release depth frame source
  SafeRelease(depthFrameSource);

  //allocate depth buffer
  m_depthBuffer = new uint16[m_depthWidth * m_depthHeight];

 //get color frame source
  IColorFrameSource* colorFrameSource;
  hr = m_sensor->get_ColorFrameSource(&colorFrameSource);
  if(FAILED(hr))
  {
    printf("Failed to get color frame source!\n");
    exit(10);
  }

  //get color frame reader
  hr = colorFrameSource->OpenReader(&m_colorFrameReader);
  if(FAILED(hr))
  {
    printf("Failed to open color frame reader!\n");
    exit(10);
  }
  
  //release the color frame source
  SafeRelease(colorFrameSource);

  //allocate color buffer
  m_colorBuffer = new uint32[1920 * 1080];

  //get the coordinate mapper
  hr = m_sensor->get_CoordinateMapper(&m_coordinateMapper);
  if(FAILED(hr))
  {
    printf("Failed to get coordinate mapper!\n");
    exit(10);
  }

  //get the body count
  hr = m_sensor->get_BodyFrameSource(&m_bodyFrameSource);
  if (FAILED(hr))
  {
	  printf("Failed to get body frame source!\n");
	  exit(10);
  }

  hr = m_bodyFrameSource->OpenReader(&m_bodyFrameReader);
  if (FAILED(hr))
  {
	  printf("Failed to get body frame reader!\n");
	  exit(10);
  }

  //allocate a buffer of color space points
  m_colorSpacePoints = new ColorSpacePoint[m_depthWidth * m_depthHeight];
}

void App::Tick(float deltaTime)
{
  //put update and drawing stuff here

  HRESULT hr;

  //depth stuff
  IDepthFrame* depthFrame;
  hr = m_depthFrameReader->AcquireLatestFrame(&depthFrame);
  if(FAILED(hr)) return;

  BOOLEAN need = true;
  int32 bodyCount = 0;

  std::cout << bodyCount << ":";
  m_bodyFrameSource->get_BodyCount(&bodyCount);

  std::cout << bodyCount << "\n" << std::endl;

  printf("Copying data!\n");
  hr = depthFrame->CopyFrameDataToArray(
    m_depthWidth * m_depthHeight, m_depthBuffer);
  
  //size_t n = 512 * 424;
  size_t n = m_depthWidth * m_depthHeight;

  time_t now = time(0);
  struct tm tstruct;
  char buf[80];
  tstruct = *localtime(&now);

  strftime(buf, sizeof(buf), "%F %T", &tstruct);

  uint16 x_vec[512 * 424];
  uint16 y_vec[512 * 424];

  std::vector<std::string> time_vec;
  time_vec.assign(512 * 424, buf);

  for (size_t i = 0; i < n; i++) {
	  x_vec[i] = i % 512;
	  y_vec[i] = 424 - (i / 512);
	  //myfile << time_vec.at(i) << "," << x_vec[i] << "," << y_vec[i] << "," << m_depthBuffer[i] << "\n";
  }

  time_vec.clear();



  if(FAILED(hr))
  {
    SafeRelease(depthFrame);
    printf("oh no, something went wrong while copying!\n");
    return;
  }
  SafeRelease(depthFrame);



  //color stuff
  IColorFrame* colorFrame;
  hr = m_colorFrameReader->AcquireLatestFrame(&colorFrame);
  if(FAILED(hr))
    return;

  hr = colorFrame->CopyConvertedFrameDataToArray(
    1920 * 1080 * 4, (BYTE*)m_colorBuffer, ColorImageFormat_Bgra);
  if(FAILED(hr))
    return;

  SafeRelease(colorFrame);

  hr = m_coordinateMapper->MapDepthFrameToColorSpace(
    m_depthWidth * m_depthHeight, m_depthBuffer,
    m_depthWidth * m_depthHeight, m_colorSpacePoints);
  if(FAILED(hr))
  {
    printf("Oh no! Failed map the depth frame to color space!\n");
    return;
  }


  //copy depth data to the screen
  for(int i = 0; i < m_depthWidth * m_depthHeight; ++i)
  {
    ColorSpacePoint csp = m_colorSpacePoints[i];
    int ix = (int)csp.X;
    int iy = (int)csp.Y;

    if(ix >= 0 && ix < 1920 && iy >= 0 && iy < 1080)
      m_pixelBuffer[i] = m_colorBuffer[ix + iy * 1920];
    else
      m_pixelBuffer[i] = 0xff0000;

    //uint16 d = m_depthBuffer[i];
    //uint8 a = d & 0xff;
    //m_pixelBuffer[i] = (a << 16) | (a << 8) | a;
  }
}

void App::Shutdown()
{
  //put cleaning up stuff here

  delete[] m_colorBuffer;
  SafeRelease(m_colorFrameReader);

  delete[] m_depthBuffer;
  SafeRelease(m_depthFrameReader);
  SafeRelease(m_sensor);
}
