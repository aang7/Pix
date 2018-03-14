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
#include <GLFW/glfw3.h>
#include<pthread.h>
#include<string>
#include <sstream>
#include <iostream>
#include "tinyfiledialogs.h"



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

static cv::Mat image2;
static bool show_tester_image = false;
//static VideoCapture* inVid;

using namespace std;
using namespace ImGui;

struct OpenCVImage
{

    GLuint texture;
    cv::Mat mat;
    bool open;
    string name;
    
    OpenCVImage() {
	texture = 0;
    }
    
    void LoadCVMat(cv::Mat frame, int internal_format = GL_BGR, int format_to_convert = GL_RGB) {

	if (frame.empty())
	    return;
	
	if (!texture)
	{
	    mat = frame; // maybe i should copy that frame (clone it)

	    glGenTextures(1, &texture);
	    glBindTexture(GL_TEXTURE_2D, texture);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    
	    // Set texture clamping method
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	    glTexImage2D(GL_TEXTURE_2D, 0, format_to_convert, mat.cols, mat.rows, 0, internal_format, GL_UNSIGNED_BYTE, mat.ptr());
	    
	}else
	    cout << "We HAVE SOMETHING AT TEXTURE" << "\n";
    }


    void UpdateMat(cv::Mat& frame ) {
	// image must have same size
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.cols, frame.rows, GL_RGB,
			GL_UNSIGNED_BYTE, frame.ptr());
	// if does not have the same size then i need to recreate the texture from scratch
	//could be something like
	/*if (!(frame.size() == mat.size())) {
		Clear();
		mat.release(); // maybe this is redundant
		LoadCVMat(frame);
		
	} */
    }

    // clear texture and realease all memory associated with it
    void Clear() { glDeleteTextures(1, &texture); texture = 0; }

    GLuint * getTexture() { return &texture; }

    // return mat if is not empty otherwise return null
    cv::Mat* GetMat() {

	if (!mat.empty())
	{
	    if (mat.data)
		return &mat;
	    else
		return NULL;
	} else
	    return NULL;
	
    }

    void switchOpen(){
	if (open)
	    open = false;
	else
	    open = true;
    }

    bool * getOpen() { return &open; }

    void SetName(const char* nme) {
	name = std::string(nme);
    }

    //then GetName

    const char * GetName(){
	return name.c_str();
    }
};

static void showImage(const char *windowName,bool *open, GLuint *textura) {

    if (*open)
	{

	    if (ImGui::Begin(windowName, open, ImGuiWindowFlags_ResizeFromAnySide))
		{
		    ImVec2 pos = ImGui::GetCursorScreenPos(); // actual position
		    ImGui::GetWindowDrawList()->AddImage((void*)*textura, pos, ImVec2(ImGui::GetContentRegionAvail().x + pos.x, ImGui::GetContentRegionAvail().y  + pos.y));
		    
		}	
	    ImGui::End();
	    
	}
}


char const * lTheOpenFileName;
char const * lFilterPatterns[2] = { "*.jpg", "*.png" };

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
    else {cout << "file choosed: " << lTheOpenFileName << endl; return (void *)lTheOpenFileName;}
    
    
}

void ImGui::ShowPixui(bool *p_open)
{
    static std::vector<OpenCVImage> data;
    static bool window1 = true;
    static bool window2 = false;
    static int  selectedItem = -1;
    static int actualitem = -1;


    ImGui::ShowMetricsWindow(&window2);
    if (ImGui::Begin("nombre", p_open, ImVec2(90, 100), ImGuiWindowFlags_MenuBar ))
    {

	    if (ImGui::Button("Create new OpenCVImage"))
		{
		    data.push_back(OpenCVImage());
		    static pthread_t t;
		
		    //Launch a thread
		    pthread_create(&t, NULL, call_from_thread, NULL);
		    
		    void* temp = NULL;
		    pthread_join(t, &temp); 

		    if (temp != NULL) {
			const char* returnValue = (const char *) temp;
			cout << "RETURNED VALUE: " << returnValue << endl;
			cv::Mat mmm = cv::imread(returnValue); 
			data.back().LoadCVMat(mmm);
			data.at(data.size() - 1).SetName(returnValue);
			mmm.release();
		    }
		    
		}

	    
	    for(int i = 0;i < data.size(); i++)
	    	{
		    
		    ImGui::PushID(i);
		    
		    if (ImGui::Selectable(data.at(i).GetName(), data.at(i).getOpen()))
			selectedItem = i;
		    
		    ImGui::PopID();
		    ImGui::Text("value: %s", *(data.at(i).getOpen()) ? "true" : "false");
	    	}
	    
	    
	    static OpenCVImage myglopencv;
	    static OpenCVImage testImage;

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
			    testImage.LoadCVMat(data.at(selectedItem).GetMat()->clone());
			    
			}

		    
		}

	    if (show_tester_image) {
		showImage("Tester", &show_tester_image, testImage.getTexture());
	    }



	    
		static int value = 0;
		static int oldvalue = 0;

		ImGui::SliderInt("Gaussian Blur", &value, 0, 101);
		if (value%2 == 0)
		    value += 1; //we need a non number
		if (show_tester_image) {
		    if (value != oldvalue) {
			oldvalue = value;
			static cv::Mat smooth;
			if (testImage.GetMat() != NULL)
			    {
				GaussianBlur( *(testImage.GetMat()) , smooth, cv::Size( value, value ), 0, 0 );
				testImage.UpdateMat(smooth);
				
			    }
			
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

void getVideoCapture() {
    
}

#else

void ImGui::ShowDemoWindow(bool*) {}
void ImGui::ShowUserGuide() {}
void ImGui::ShowStyleEditor(ImGuiStyle*) {}

#endif
