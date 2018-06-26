// Copyright 2016 <Muharrem Kantar>
// All rights reserved.

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cmath>
#include <ctime>
#include <vector>

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

Mat get_concatenated(Mat left, Mat right);
void on_mouse(int event, int x, int y, int flags, void* param);
void draw_point(int x, int y, void *m);
void draw_line(void *m);
bool is_corresponded(bool left, bool right);
bool is_in_circle_l(int x, int y, void* m);
bool is_in_circle_r(int x, int y, void* m);
void reconstruct_image_l(int x, int y, void* m);
void reconstruct_image_r(int x, int y, void* m);

/** GLOBALS **/
vector<int> l;
vector<int> r;
int _width_ = 0;
int standby_left[2];
int standby_right[2];

Mat original;
 
int main(int argc, char *argv[]) {

  if (argc != 3) {
    cout << "Usage : <image_left> <image_right>" << endl;
    exit(EXIT_FAILURE);
  }

  Mat left  = imread(argv[1], 1);
  Mat right = imread(argv[2], 1);

  if (!left.data && !right.data) {
    cout << "Could not open file(s)" << endl;
    exit(EXIT_FAILURE);
  }

  Mat concatenated = get_concatenated(left, right);
  original = concatenated.clone();

  namedWindow("Concatenated image", WINDOW_AUTOSIZE);

  setMouseCallback("Concatenated image", on_mouse, &concatenated);

  imshow("Concatenated image", concatenated);

  waitKey(0);

  return EXIT_SUCCESS;
}

Mat get_concatenated(Mat left, Mat right) {
  Size left_img  = left.size();
  Size right_img = right.size();

  int height     = 0;
  int width      = left_img.width + right_img.width;
  _width_        = left_img.width;

  if (left_img.height <= right_img.height) {
    height = left_img.height;
  } else {
    height = right_img.height;
  }

  Mat concatenated = Mat::zeros(height, width, CV_8UC3);

  for(int y = 0; y < concatenated.rows; ++y) {
    for(int x = 0; x < concatenated.cols; ++x) {
      if (x < left.cols) {
        Vec3b color = left.at<Vec3b>(y, x);
        concatenated.at<Vec3b>(y,x) = color;
      } else {
        Vec3b color = right.at<Vec3b>(y, x - left.cols);
        concatenated.at<Vec3b>(y,x) = color;
      }
    }
  }

  return concatenated;

}

void on_mouse(int event, int x, int y, int flags, void* param) {
  static bool isLeftClicked  = false;
  static bool isRightClicked = false;

  static bool isLeftStandBy  = false;
  static bool isRightStandBy = false;
  static int  click_counter  = 0;
  
  switch (event) {
    case EVENT_LBUTTONDBLCLK:
      if ((x < _width_) && (isLeftClicked != true) && ((isLeftStandBy != true) && (isRightStandBy != true))) {
        l.push_back(x);
        l.push_back(y);
        draw_point(x, y, param);
        isLeftClicked = true;
        if (is_corresponded(isLeftClicked, isRightClicked)) {
          draw_line(param);
          isLeftClicked = false;
          isRightClicked = false;
        }
      } else if ((x >= _width_) && (isRightClicked != true) && (isRightStandBy != true && isLeftStandBy != true)) {
        r.push_back(x);
        r.push_back(y);
        draw_point(x, y, param);
        isRightClicked = true;
        if (is_corresponded(isLeftClicked, isRightClicked)) {
          draw_line(param);
          isLeftClicked = false;
          isRightClicked = false;
        }
      } else if((x < _width_) && (isLeftStandBy == true)) {
        cout << "Begin reconstruction of image" << endl;
        reconstruct_image_l(x, y, param);
        isLeftStandBy =  false;
      } else if((x >= _width_) && (isRightStandBy == true)) {
        cout << "Begin resconstruction of image" << endl;
        reconstruct_image_r(x, y, param);
        isRightStandBy = false;
      }
      
      cout << "Left button of the mouse is double clicked - position (" << x << "," << y << ")" << endl;
      break;
      case EVENT_RBUTTONDOWN:
        if ((x < _width_ ) && (isLeftStandBy != true) && (isRightStandBy != true) && !l.empty() && !r.empty()) {
          isLeftStandBy = is_in_circle_l(x, y, param);
        } else if ((x >= _width_) && (isRightStandBy != true) && (isLeftStandBy != true) && !l.empty() && !r.empty()) {
          isRightStandBy = is_in_circle_r(x, y, param);
        }
        
        cout << "Right button of the mouse is clicked - position (" << x << "," << y << ")" << endl;
      break;

    default:
      break;
  }

}

void draw_point(int x, int y, void *m) {
  Mat *p = static_cast<Mat*> (m);
  circle(*p, Point(x, y), 0, Scalar(255, 0, 0), 1, 8, 0);
  circle(*p, Point(x, y), 9, Scalar(255, 255, 0), 1, 8, 0);
  imshow("Concatenated image", *p);
}

void draw_line(void *m) {
  Mat *p = static_cast<Mat*> (m);
  Point pt1;
  Point pt2;
  if (!l.empty() && !r.empty()) {
    pt1.y = l.at(l.size() - 1);
    pt1.x = l.at(l.size() - 2);
    pt2.y = r.at(r.size() - 1);
    pt2.x = r.at(r.size() - 2);
  } else {
    cout << "An unexpected exception occurred." << endl;
    cout << "Exiting..." << endl;
    exit(EXIT_FAILURE);
  }
  
  line(*p, pt1, pt2, Scalar(0, 255, 0), 1, 8, 0);
  cout << l.at(l.size() - 2) << "," << l.at(l.size() - 1) << endl;
  cout << r.at(r.size() - 2) << "," << r.at(r.size() - 1) << endl;
  
  imshow("Concatenated image", *p);
  
}

bool is_corresponded(bool left, bool right) {
  return left && right;
}

bool is_in_circle_l(int x, int y, void* m) {
  Mat *p = static_cast<Mat*> (m);
  int N = 7;
  bool isMarked = false;
  cout << "Search on left image" << endl;
  int i = 0, j = 0;
  for(i = 0, j = 1; (i < l.size()) && (j < l.size()); i = i + 2, j = j + 2) {
      int x_circle[N] = {l.at(i) - 3, l.at(i) - 2, l.at(i) - 1, l.at(i), l.at(i) + 1, l.at(i) + 2, l.at(i) + 3};
      int y_circle[N] = {l.at(j) - 3, l.at(j) - 2, l.at(j) - 1, l.at(j), l.at(j) + 1, l.at(j) + 2, l.at(j) + 3};
      cout << "Points initialized on right image" << endl;
      for(int k = 0; k < N; k++) {
        for(int ll = 0; ll < N; ll++) {
          if((x_circle[k] == x) && (y_circle[ll] == y)) {
            circle(*p, Point(l.at(i), l.at(j)), 0, Scalar(0, 0, 255), 3, 8, 0);
            standby_left[0] = i;
            standby_left[1] = j;
            imshow("Concatenated image", *p);
            isMarked = true;
          }
        }
      }
      
    
  }
  
    
  return isMarked;
}

bool is_in_circle_r(int x, int y, void* m) {
  Mat *p = static_cast<Mat*> (m);
  cout << "Search on right image" << endl;
  int N = 7;
  bool isMarked = false;
  int i = 0, j = 0;
  for(i = 0, j = 1; (i < r.size()) && (j < r.size()); i = i + 2, j = j + 2) {
      int x_circle[N] = {r.at(i) - 3, r.at(i) - 2, r.at(i) - 1, r.at(i), r.at(i) + 1, r.at(i) + 2, r.at(i) + 3};
      int y_circle[N] = {r.at(j) - 3, r.at(j) - 2, r.at(j) - 1, r.at(j), r.at(j) + 1, r.at(j) + 2, r.at(j) + 3};
      cout << "Points initialized on right image" << endl;
      for(int k = 0; k < N; k++) {
        for(int ll = 0; ll < N; ll++) {
          if((x_circle[k] == x) && (y_circle[ll] == y)) {
            circle(*p, Point(r.at(i), r.at(j)), 0, Scalar(0, 0, 255), 3, 8, 0);
            standby_right[0] = i;
            standby_right[1] = j;
            imshow("Concatenated image", *p);
            isMarked = true;
          }
        }
      }
      
    
  }
    
    return isMarked;
}

void reconstruct_image_l(int x, int y, void* m) {
  Mat* p = static_cast<Mat*> (m);
  *p = original.clone();

  l.at(standby_left[0]) = x;
  l.at(standby_left[1]) = y;
  
  int size = l.size();
  Point pt1;
  Point pt2;
  for(int i = 0; i < size - 1; i = i + 2) {
    pt1.x = l.at(i);
    pt1.y = l.at(i + 1);
    pt2.x = r.at(i);
    pt2.y = r.at(i + 1);

    circle(*p, pt1, 0, Scalar(255, 0, 0), 1, 8, 0);
    circle(*p, pt1, 9, Scalar(255, 255, 0), 1, 8, 0);

    circle(*p, pt2, 0, Scalar(255, 0, 0), 1, 8, 0);
    circle(*p, pt2, 9, Scalar(255, 255, 0), 1, 8, 0);

    line(*p, pt1, pt2, Scalar(0, 255, 0), 1, 8, 0);
    
  }


  imshow("Concatenated image", *p);
  
}

void reconstruct_image_r(int x, int y, void* m) {
  Mat *p = static_cast<Mat*> (m);
  *p = original.clone();

  r.at(standby_right[0]) = x;
  r.at(standby_right[1]) = y;
  
  int size = r.size();
  Point pt1;
  Point pt2;
  for(int i = 0; i < size - 1; i = i + 2) {
    pt1.x = l.at(i);
    pt1.y = l.at(i + 1);
    pt2.x = r.at(i);
    pt2.y = r.at(i + 1);

    circle(*p, pt1, 0, Scalar(255, 0, 0), 1, 8, 0);
    circle(*p, pt1, 9, Scalar(255, 255, 0), 1, 8, 0);

    circle(*p, pt2, 0, Scalar(255, 0, 0), 1, 8, 0);
    circle(*p, pt2, 9, Scalar(255, 255, 0), 1, 8, 0);

    line(*p, pt1, pt2, Scalar(0, 255, 0), 1, 8, 0);
    
  }

  imshow("Concatenated image", *p);
}
