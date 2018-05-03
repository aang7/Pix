
#include <opencv/cv.h>
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include <GLFW/glfw3.h>
#include<string>
#include <sstream>
#include <iostream>

using namespace cv;
using namespace std;

struct OpenCVImage
{

    GLuint texture;
    cv::Mat mat;
    bool open;
    string name;
    
    OpenCVImage() {
	texture = 0;
    }
    
    void LoadCVMat(cv::Mat frame, bool change = true) {

	if (frame.empty())
	    return;

	if (!texture)
	{
	    mat = frame; // maybe i should copy that frame (clone it)

	    if (change)
		cv::cvtColor(mat, mat, CV_BGR2RGB);
	    
	    glGenTextures(1, &texture);
	    glBindTexture(GL_TEXTURE_2D, texture);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    
	    // Set texture clamping method
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mat.cols, mat.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, mat.ptr());
	    
	}else
	    cout << "We HAVE SOMETHING AT TEXTURE" << "\n";
    }


    void UpdateMat(cv::Mat& frame, bool change_color_order = false) {

	// if does not have the same size then i need to recreate the texture from scratch
	//could be something like
	 if (!(frame.size() == mat.size())) {
	    Clear();
	    mat.release(); // maybe this is redundant
	    LoadCVMat(frame);
	 } 
	
	
	if (change_color_order)
	    cv::cvtColor(frame, frame, CV_BGR2RGB);

	// image must have same size
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.cols, frame.rows, GL_RGB,
			GL_UNSIGNED_BYTE, frame.ptr());
	
	
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

    // this function could be omitted using the above function
    // but we had to change the use of it ( the above)
    cv::Mat* GetMatDirection() {
	return &mat;
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
