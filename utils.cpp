#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <map>
#include <string>

#include "distance.h"
#include "utils.h"



using namespace cv;
using namespace std;




Mat saturate(Mat src, double low, double up) {
    Mat src_saturation;
    if (src.channels() == 1) {     
        src_saturation.create(src.rows,1, src.type());
        for (int i = 0; i < src.rows; i++) {
            bool saturation = true;
            bool saturation_ij = true;
            double* data = src.ptr<double>(i);
            for (int j = 0; j < src.cols; j++) {
                if ((data[j]<up) && (data[j] > low)) {
                    saturation_ij = true;
                }
                else {
                    cout << i<<" "<<j<<endl;
                    saturation_ij = false;
                    break;
                }
               // saturation = saturation && saturation_ij;
            }
            src_saturation.at<double>(i,0)= saturation_ij;
        }

    }
    if (src.channels() == 3) {
        src_saturation.create(src.size(), CV_64FC1);
        cout << up<<" up low "<<low << endl;
        for (int i = 0; i < src.rows; i++) {
            for (int j = 0; j < src.cols; j++) {
                bool saturation_ij = true;
                for (int m = 0; m < 3; m++) {
                   // cout << src.at<Vec3d>(i, j)[m] << endl;
                    if ((src.at<Vec3d>(i, j)[m] < up) && (src.at<Vec3d>(i, j)[m] > low)){
                        
                        cout << src.at<Vec3d>(i, j)[m] << endl;
                        cout <<"234 "<< saturation_ij << endl;
                        continue;
                    }
                    else {
                        saturation_ij = false;
                       // cout << saturation_ij <<endl;
                        break;
                    }
                }
                cout << "123" << endl;
                src_saturation.at<double>(i, j) = (double)saturation_ij;
            }
        }

    }
    return src_saturation;
}
    
Mat xyz2grayl(Mat xyz) {
    Mat dst;

    int height = xyz.rows;
    int width = xyz.cols;
    int nc = xyz.channels();

    if (nc == 1) {
        Mat dst = xyz.colRange(1, 2).clone(); 
    }
    else if (nc == 3) {
        vector<Mat> channels;
        split(xyz, channels);
        dst = channels[1];
    }
    return dst;

}

Mat xyz2lab(Mat xyz, IO io) {
    map <IO, vector<double>> xyz_ref_white = get_xyz_ref_white();
    vector<double> xyz_ref_white_io = xyz_ref_white[io];
    
    float Xn = xyz_ref_white_io[0];
    float Yn = xyz_ref_white_io[1];
    float Zn = xyz_ref_white_io[2];
    Mat lab;
    lab.create(xyz.size(), xyz.type());
    if (xyz.channels() == 1) {
        for (int i = 0; i < xyz.rows; i++) {
            double* datasrc = xyz.ptr<double>(i);
            double x = datasrc[0];
            double y = datasrc[1];
            double z = datasrc[2];
            double L, a, b;
            f_xyz2lab(x, y, z, L, a, b, Xn, Yn, Zn);
            //cout << x << y << z << endl;
          //  cout << L <<" "<< a <<" "<< b << endl;
          //  cout << lab.size() << endl;
            lab.at<double>(i, 0) = L;
            lab.at<double>(i, 1) = a;
            lab.at<double>(i, 2) = b;

        }
        return lab;
    }
    else if(xyz.channels() == 3){
      
        Mat channel(cv::Size(xyz.rows, xyz.cols), CV_32FC3);
        for (int i = 0; i < xyz.rows; i++) {
            for (int j = 0; j < xyz.cols; j++) {
                double x = xyz.at<Vec3d>(i, j)[0];
                double y = xyz.at<Vec3d>(i, j)[1];
                double z = xyz.at<Vec3d>(i, j)[2];
                double L, a, b;
                f_xyz2lab(x, y, z, L, a, b, Xn, Yn, Zn);
                lab.at<Vec3d>(i, j)[0] = L;
                lab.at<Vec3d>(i, j)[1] = a;
                lab.at<Vec3d>(i, j)[2] = b;
            }
        }
          
        return lab;
    }
    else {
        return xyz;
    }
    
}

 void f_xyz2lab(double  X, double  Y, double Z,
    double& L, double& a, double& b, double Xn, double Yn, double Zn)
{
    // CIE XYZ values of reference white point for D65 illuminant
   // static const float Xn = 0.950456f, Yn = 1.f, Zn = 1.088754f;

    // Other coefficients below:
    // 7.787f    = (29/3)^3/(29*4)
    // 0.008856f = (6/29)^3
    // 903.3     = (29/3)^3

    double x = X / Xn, y = Y / Yn, z = Z / Zn;
   // cout <<x <<" "<<y<<" "<<z<< endl;
    auto f = [](double t) { return t > 0.008856 ? std::cbrtl(t) : (7.787 * t + 16.0 / 116.0); };

    double fx = f(x), fy = f(y), fz = f(z);
    cout << fx << " " << fy << " " << fz << endl;
    L = y > 0.008856 ? (116.0 * std::cbrtl(y) - 16.0) : (903.3 * y);
  
    a = 500.0 * (fx - fy);
    b = 200.0 * (fy - fz);
}
 double  r_revise(double x) {
     if (x > 6.0 / 29.0)
         x = pow(x, 3.0);
     else
         x = (x - 16.0 / 116.0) * 3 * powl(6.0 / 29.0, 2);
     return x;
 }
void f_lab2xyz(double l, double a, double b, double& x, double& y, double& z, double Xn, double Yn, double Zn ) {
     double Y = (l + 16.0) / 116.0;
     double X = Y + a / 500.0;
     double Z = Y - b / 200.0;

     x = r_revise(X);
     y = r_revise(Y);
     z = r_revise(Z);
     if (z < 0) {
         z = 0;
     }
     x = x * Xn;
     y = y * Yn;
     z = z * Zn;
 }

Mat lab2xyz(Mat lab, IO io) {
    map <IO, vector<double>> xyz_ref_white = get_xyz_ref_white();
    vector<double> xyz_ref_white_io = xyz_ref_white[io];
    float Xn = xyz_ref_white_io[0];
    float Yn = xyz_ref_white_io[1];
    float Zn = xyz_ref_white_io[2];
    Mat xyz;
    xyz.create(lab.size(), lab.type());
    if (lab.channels() == 1) {
       
        for (int i = 0; i < xyz.rows; i++) {
            double* datasrc = lab.ptr<double>(i);
            double l = datasrc[0];
            double a = datasrc[1];
            double b = datasrc[2];
            double x, y, z;
       
            f_lab2xyz(l, a, b, x, y, z, Xn, Yn, Zn);
          
            xyz.at<double>(i, 0) = x;
            xyz.at<double>(i, 1) = y;
            xyz.at<double>(i, 2) = z;

        }
        return xyz;
    }
    else if (lab.channels() == 3) {
      //  cout << xyz.type() << endl;
      //  cout <<"chan "<< xyz.channels() << endl;
      //  xyz.create(lab.size(), lab.type());
        for (int i = 0; i < lab.rows; i++) {
            for (int j = 0; j < lab.cols; j++) {
                double l = lab.at<Vec3d>(i, j)[0];
                //cout << l <<" hekpohkpo "<< endl;
                double a = lab.at<Vec3d>(i, j)[1];
                double b = lab.at<Vec3d>(i, j)[2];
                double x, y, z;
               
                f_lab2xyz(l, a, b, x, y, z, Xn, Yn, Zn);
              //  cout << l << a << b << endl;
                xyz.at<Vec3d>(i, j)[0] = x;
                xyz.at<Vec3d>(i, j)[1] = y;
                xyz.at<Vec3d>(i, j)[2] = z;
            }
        }

        return xyz;
    }
    else {
        return lab;
    }

}


Mat rgb2gray(Mat rgb) {
    Mat togray = (Mat_<double>(3, 1) << 0.2126, 0.7152, 0.0722 );
    
    Mat gray ;
    if (rgb.channels() == 1) {
        gray.create((rgb.rows), 1, rgb.type());
        for (int i = 0; i < rgb.rows; i++) {
            gray.row(i) = rgb.row(i) * togray;
        }
    }
    else if (rgb.channels() == 3) {
        gray.create(rgb.rows, rgb.cols, CV_64FC1);
        for (int i = 0; i < rgb.rows; i++) {
            for (int j = 0; j < rgb.cols; j++) {
                double res1 = rgb.at<Vec3d>(i, j)[0] * togray.at<double>(0, 0);
                
                double res2 = rgb.at<Vec3d>(i, j)[1] * togray.at<double>(1, 0);
               
                double res3 = rgb.at<Vec3d>(i, j)[2] * togray.at<double>(2, 0);
                
                gray.at<double>(i,j)= res1+ res2 + res3;
               
            }
        }
    }
   
    
    return gray;
}
      

Mat xyz2xyz(Mat xyz, IO sio, IO dio) {
    if (sio.m_illuminant == dio.m_illuminant && sio.m_observer==dio.m_observer) {
        return xyz;
    }
    else {
        Mat cam(IO, IO, string);
        Mat cam_M=cam(sio,dio, "Bradford");
        Mat cam_res;
        cam_res.create(xyz.size(), xyz.type());
        if (xyz.channels() == 1) {
            cam_res = xyz * (cam_M.t());
            
        }
        else if(xyz.channels() == 3){
            for (int i = 0; i < xyz.rows; i++) {
               // Mat res_row;
                //res_row.create(xyz.rows, 3, CV_32FC1);
                for (int j = 0; j < xyz.cols; j++) {
                    for (int m = 0; m < 3; m++) {
                        double res1 = xyz.at<Vec3d>(i, j)[0] * cam_M.at<double>(m, 0);
                        double res2 = xyz.at<Vec3d>(i, j)[1] * cam_M.at<double>(m, 1);
                        double res3 = xyz.at<Vec3d>(i, j)[2] * cam_M.at<double>(m, 2);
                        cam_res.at<Vec3d>(i, j)[m] = res1 + res2 + res3;
                    }
                   
                }
            }
            
        }
        return cam_res;
    }
    }

Mat lab2lab(Mat lab, IO sio, IO dio) {
    if (sio.m_illuminant == dio.m_illuminant && sio.m_observer == dio.m_observer) {
        return lab;
    }
    else {
        return xyz2lab(xyz2xyz(lab2xyz(lab, sio), sio, dio), dio);
    }     
}
    
        

double gamma_correction_f(double f, double gamma) {
    double k;
    if (f >= 0) {
        k = pow(f, gamma);
    }
    else{
        k =-pow((-f), gamma);
}
    return k;
}


Mat gamma_correction(cv::Mat& src, float K) {
    Mat dst;
    src.copyTo(dst);
    //gamma_correction_f_M = gamma_correction_f;
    for (int row = 0; row < src.rows; row++) {
        for (int col = 0; col < src.cols; col++) {
            if (src.channels() == 1) {
                dst.at<double>(row, col) = gamma_correction_f(src.at<double>(row, col), K);
            }
            else if (src.channels() == 3) {
                dst.at<Vec3d>(row, col)[0] = gamma_correction_f(src.at<Vec3d>(row, col)[0], K);
                dst.at<Vec3d>(row, col)[1] = gamma_correction_f(src.at<Vec3d>(row, col)[1], K);
                dst.at<Vec3d>(row, col)[2] = gamma_correction_f(src.at<Vec3d>(row, col)[2], K);
            }
        }
    }
    return dst;
}
