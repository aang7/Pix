#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#include <ctype.h>          // toupper, isprint
#include <math.h>           // sqrtf, powf, cosf, sinf, floorf, ceilf
#include <stdio.h>          // vsnprintf, sscanf, printf
#include <stdlib.h>         // NULL, malloc, free, atoi
#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>         // intptr_t
#else
#include <stdint.h>         // intptr_t
#endif
#include <opencv2/core/utility.hpp>
#include <opencv/cv.h>
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include<opencv2/opencv.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <GLFW/glfw3.h>
#include<pthread.h>
#include<string>
#include <sstream>
#include <iostream>
#include "tinyfiledialogs.h"
#include "imageutils.h"


#ifdef _MSC_VER
#pragma warning (disable: 4996) // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#define snprintf _snprintf
#endif
#ifdef __clang__
#pragma clang diagnostic ignored "-Wold-style-cast"             // warning : use of old-style cast                              // yes, they are more terse.
#pragma clang diagnostic ignored "-Wdeprecated-declarations"    // warning : 'xx' is deprecated: The POSIX name for this item.. // for strdup used in demo code (so user can copy & paste the code)
#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast"   // warning : cast to 'void *' from smaller integer type 'int'
#pragma clang diagnostic ignored "-Wformat-security"            // warning : warning: format string is not a string literal
#pragma clang diagnostic ignored "-Wexit-time-destructors"      // warning : declaration requires an exit-time destructor       // exit-time destruction order is undefined. if MemFree() leads to users code that has been disabled before exit it might cause problems. ImGui coding style welcomes static/globals.
#if __has_warning("-Wreserved-id-macro")
#pragma clang diagnostic ignored "-Wreserved-id-macro"          // warning : macro name is a reserved identifier                //
#endif
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"          // warning: cast to pointer from integer of different size
#pragma GCC diagnostic ignored "-Wformat-security"              // warning : format string is not a string literal (potentially insecure)
#pragma GCC diagnostic ignored "-Wdouble-promotion"             // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"                   // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#if (__GNUC__ >= 6)
#pragma GCC diagnostic ignored "-Wmisleading-indentation"       // warning: this 'if' clause does not guard this statement      // GCC 6.0+ only. See #883 on GitHub.
#endif
#endif

// Play it nice with Windows users. Notepad in 2017 still doesn't display text data with Unix-style \n.
#ifdef _WIN32
#define IM_NEWLINE "\r\n"
#else
#define IM_NEWLINE "\n"
#endif

#define IM_MAX(_A,_B)       (((_A) >= (_B)) ? (_A) : (_B))

#if !defined(IMGUI_DISABLE_OBSOLETE_FUNCTIONS) && defined(IMGUI_DISABLE_TEST_WINDOWS) && !defined(IMGUI_DISABLE_DEMO_WINDOWS)   // Obsolete name since 1.53, TEST->DEMO
#define IMGUI_DISABLE_DEMO_WINDOWS
#endif

#if !defined(IMGUI_DISABLE_DEMO_WINDOWS)

GLuint mat_to_tex(const cv::Mat&);
void load_opencv_Mat(cv::Mat, GLuint *);
void findCircles(cv::Mat& src, int entre, int threshold, int threshold_center);
cv::Mat& myFindContours(cv::Mat mat);

static cv::Mat image2;
static cv::Mat tbaseimage; // base tester image
static bool show_tester_image = false;
static cv::VideoCapture camera;
static int thresh = 100;
static int cam_id = 1; //my laptop webcam if we had another webcam connected the cam_id would be = 1
cv::RNG rng(12345);
char const * lTheOpenFileName;
char const * lFilterPatterns[2] = { "*.jpg", "*.png" };
const char *filepath = NULL;


using namespace std;
using namespace ImGui;
using namespace cv;


static void showImage(const char *windowName,bool *open, GLuint *textura) {

    if (*open)
	{

	    ImGui::SetNextWindowBgAlpha(0.4f); // Transparent background
	    if (ImGui::Begin(windowName, open, ImGuiWindowFlags_ResizeFromAnySide))
		{
		    ImVec2 pos = ImGui::GetCursorScreenPos(); // actual position
		    ImGui::GetWindowDrawList()->AddImage((void*)*textura, pos, ImVec2(ImGui::GetContentRegionAvail().x + pos.x, ImGui::GetContentRegionAvail().y  + pos.y));
		    
		}	
	    ImGui::End();
	    
	}
}



void *call_from_thread(void *) {
    lTheOpenFileName = tinyfd_openFileDialog(
					     "let us read the password back",
					     "",
					     2,
					     lFilterPatterns,
					     NULL,
					     0);
    if (! lTheOpenFileName)
	{
	    tinyfd_messageBox(
			      "Error",
			      "Open file name is NULL",
			      "ok",
			      "error",
			      1);
	    return NULL;
	    
	}
    else {
	cout << "file choosed: " << lTheOpenFileName << endl;
	filepath = lTheOpenFileName;
    }
    
    return NULL;
}


void ImGui::ShowPixui(bool *p_open)
{
    static std::vector<OpenCVImage> data;
    static bool window2 = false;
    static int  selectedItem = -1;
    static int actualitem = -1;


    ImGui::ShowMetricsWindow(&window2);
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background
    if (ImGui::Begin("nombre", p_open, ImVec2(90, 100) ))
    {

	    if (ImGui::Button("Create new OpenCVImage"))
		{
		    static pthread_t t;
		    
		    //Launch a thread
		    pthread_create(&t, NULL, call_from_thread, NULL);
		    
		}

	    if (filepath != NULL)
		{
		    data.push_back(OpenCVImage());
		    cv::Mat mmm = cv::imread(filepath); 
		    data.back().LoadCVMat(mmm);
		    data.at(data.size() - 1).SetName(filepath);
		    mmm.release();
		    filepath = NULL;
		}

	    
	    for(int i = 0;i < data.size(); i++)
	    	{
		    
		    ImGui::PushID(i);
		    
		    if (ImGui::Selectable(data.at(i).GetName(), data.at(i).getOpen()))
			selectedItem = i;
		    
		    ImGui::PopID();
		    ImGui::Text("value: %s", *(data.at(i).getOpen()) ? "true" : "false");
	    	}
	    
	    
	    static OpenCVImage testImage;
	    static OpenCVImage webcamImage;

	    if (ImGui::Selectable("Show tester image", &show_tester_image))
		{

		    
		    if (actualitem != selectedItem && selectedItem != -1)
			{

			    actualitem = selectedItem;

			    if (testImage.GetMat() != NULL) {
				testImage.GetMat()->release(); // free mat memory
				testImage.Clear(); // free texture memory
			    }

			    // reload a new image
			    testImage.LoadCVMat(data.at(selectedItem).GetMat()->clone(), false);
			    
			}

		    
		}

	    static bool open_camera = false;
	    static OpenCVImage camera_image;
	    ImGui::Selectable("Camera test", &open_camera);
	    ImVec2 size(ImGui::GetWindowContentRegionMax().x*0.70, ImGui::GetWindowHeight()*0.77);
	    ImVec2 pos = ImGui::GetCursorScreenPos(); //Pocision actual
	    if (open_camera)
		{
		    cv::Mat frame;
		    
		    if (!camera.isOpened()) 
			{
			    if (camera.open(cam_id))
				{
				    camera.set(CV_CAP_PROP_FRAME_WIDTH,800);
				    camera.set(CV_CAP_PROP_FRAME_HEIGHT,640);
				}
			    else
				std::cout << "camera don't found" << std::endl;
			}
		    else
			{
			    camera.read(frame); // get the cam image and storage it in frame variable
			    if (*camera_image.getTexture() != 0)
				camera_image.UpdateMat(frame, true);
			    else
				camera_image.LoadCVMat(frame, true);

			}
		    ImGui::GetWindowDrawList()->AddImage((void*)*camera_image.getTexture(), pos, ImVec2(pos.x + size.x, pos.y + size.y));

		    
		    if (ImGui::Button("Close camera"))
			{
			    camera.release();
			    open_camera = false;
			}
		    static int webcamframe_counter = 0;
		    if (ImGui::Button("Capture frame"))
			{
			    camera.set(CV_CAP_PROP_FRAME_WIDTH,1024);
			    camera.set(CV_CAP_PROP_FRAME_HEIGHT,768);
			    if(!frame.empty())
				{
				   
				    camera.read(frame);
				    webcamframe_counter++;
				    stringstream ss;
				    ss << "Webcam frame: " << webcamframe_counter;
				    data.push_back(OpenCVImage());
				    data.back().LoadCVMat(frame);
				    data.at(data.size() - 1).SetName(ss.str().c_str());
				    camera.set(CV_CAP_PROP_FRAME_WIDTH,800);
				    camera.set(CV_CAP_PROP_FRAME_HEIGHT, 640);

				    //webcamImage.LoadCVMat(frame, false);
				}
			}
		    
		    //ImGui::Selectable("Show Webcam Frame", webcamImage.getOpen());
		    //showImage("Webcam frame", webcamImage.getOpen(), webcamImage.getTexture());
		    		    
		}
	    
	    if (show_tester_image) {
		showImage("Tester", &show_tester_image, testImage.getTexture());
	    }

	    
		static int value = 0;
		static int oldvalue = 0;
		static int entre = 8,  threshold = 90, threshold_center = 100;
		
		ImGui::SliderInt("Gaussian Blur", &value, 0, 101);
		ImGui::SliderInt("Entre", &entre, 0, 100);
		ImGui::SliderInt("threshold", &threshold, 0, 200);
		ImGui::SliderInt("threshold center", &threshold_center, 0, 200);
		ImGui::SliderInt("canny thresh", &thresh, 0, 200);
	        
		if (value%2 == 0)
		    value += 1; //we need a non number
		if (show_tester_image) {
		    if (value != oldvalue) {
			oldvalue = value;
			
			if (testImage.GetMat() != NULL)
			    {
				cv::GaussianBlur( *(testImage.GetMat()), tbaseimage, cv::Size( value, value ), 0, 0 );
				//cv::cvtColor( *(testImage.GetMat()), *(testImage.GetMat()) , CV_RGB2GRAY ); // this is wrong
				//cv::blur( *(testImage.GetMat()), tbaseimage, Size(3,3) );
				testImage.UpdateMat(tbaseimage);
				
			    }
		    }

		    
		    if (ImGui::Button("Find circles"))
			{
			    findCircles(tbaseimage, entre, threshold, threshold_center);
			    testImage.UpdateMat(tbaseimage);
			}
		    
		    if (ImGui::Button("find contours"))
			{
			    testImage.UpdateMat(myFindContours(tbaseimage));
			}
		    
		}

		for(int i = 0; i < data.size(); i++)
		{
		    
		    ImGui::PushID(i);
		    showImage(data.at(i).GetName(), data.at(i).getOpen(), data.at(i).getTexture());
		    ImGui::PopID();
		    
		}
    
    }
    ImGui::End();
}

void findCircles(cv::Mat& src, int entre, int threshold, int threshold_center) {
      /// Convert it to gray
    cout << "entro aqui" << endl;
    cv::Mat src_gray;
    cvtColor( src, src_gray , CV_RGB2GRAY );
    
    /// Reduce the noise so we avoid false circle detection
    // GaussianBlur( src_gray, src_gray, cv::Size(9, 9), 2, 2 );
    
    vector<Vec3f> circles;
    
    /// Apply the Hough Transform to find the circles
    HoughCircles( src_gray, circles, CV_HOUGH_GRADIENT, 1, src_gray.rows/entre, threshold, threshold_center, 0, 0 );
    
    /// Draw the circles detected
    for( size_t i = 0; i < circles.size(); i++ )
	{
	    Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
	    int radius = cvRound(circles[i][2]);
	    // circle center
	    circle( src, center, 3, Scalar(0,255,0), -1, 8, 0 );
	    // circle outline
	    circle( src, center, radius, Scalar(0,0,255), 3, CV_AA, 0 );
	}
    
}


Mat drawing; /*  we need a global variable to hold the reference to show up in the UpdateMat method
		 maybe we can clone the image to create another reference but for the moment this work. */

Mat& myFindContours(cv::Mat mat) {
    Mat canny_output;
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;

    std::cout << "Channgels: "<<mat.channels();
    /// Detect edges using canny
    Canny( mat, canny_output, thresh, thresh*4, 3 );
    std::cout << "Channgels: "<<canny_output.channels();

    // Find contours needs an image of one channel so we need to catch it ..TODO
    findContours( canny_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

    /// Draw contours
    drawing = Mat::zeros( canny_output.size(), CV_8UC3 );

    for( int i = 0; i < contours.size(); i++ )
	{
	    Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
	    drawContours( drawing, contours, i, color, 1, CV_AA, hierarchy, 0, Point(0, 0) );
	}

    cout << "Tamaño del contours: " << contours[0][0] << endl;
    
    return drawing;    
}

void getVideoCapture() {
    
}

#else

void ImGui::ShowDemoWindow(bool*) {}
void ImGui::ShowUserGuide() {}
void ImGui::ShowStyleEditor(ImGuiStyle*) {}

#endif
