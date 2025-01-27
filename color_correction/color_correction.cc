/*
  Color correction using color charts.
  2025-01-06  T. Nakagawa
*/

#include <iostream>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/mcc.hpp>

#define DEBUG 0

// L*a*b* values of Charttu's color chart.
static constexpr double COLOR_CHART_LAB[24][3] = {
  {37.10,  14.00,  13.96},
  {63.98,  18.67,  17.77},
  {49.22,  -3.82, -22.22},
  {43.30, -12.86,  22.40},
  {54.80,   9.42, -24.27},
  {70.76, -32.24,   0.85},
  {62.15,  36.24,  55.49},
  {38.87,  11.28, -45.38},
  {50.00,  48.51,  16.43},
  {30.72,  21.47, -20.67},
  {71.36, -23.42,  57.64},
  {71.99,  18.44,  66.29},
  {28.34,  15.19, -50.08},
  {53.94, -39.26,  32.92},
  {42.15,  50.69,  28.01},
  {81.70,   2.47,  78.67},
  {50.55,  50.81, -14.16},
  {50.22, -30.02, -27.44},
  {95.08,  -1.15,   2.32},
  {81.65,  -0.57,   0.05},
  {67.12,  -0.73,  -0.25},
  {50.73,  -0.09,   0.21},
  {35.99,  -0.63,  -0.38},
  {21.12,  -0.14,   0.31},
};
#if 0  // Old Charttu's chart.
static constexpr double COLOR_CHART_LAB[24][3] = {
  {38.42,  14.10,  14.31},
  {64.77,  19.61,  16.33},
  {49.84,  -3.37, -22.75},
  {43.65, -12.63,  22.47},
  {55.43,   9.89, -25.15},
  {70.56, -31.68,  -0.58},
  {64.50,  36.03,  55.90},
  {40.18,  10.92, -44.10},
  {50.37,  47.71,  15.20},
  {31.19,  22.80, -20.48},
  {72.74, -22.86,  57.25},
  {71.32,  18.11,  67.88},
  {28.14,  15.06, -49.47},
  {55.54, -39.24,  31.28},
  {43.33,  51.59,  28.15},
  {81.85,   2.22,  78.87},
  {51.44,  50.48, -14.29},
  {50.31, -29.23, -27.53},
  {95.24,  -1.01,   2.52},
  {81.48,  -0.45,  -0.34},
  {66.95,  -0.95,  -0.35},
  {50.38,   0.31,  -0.41},
  {36.70,   0.02,  -0.16},
  {21.05,  -0.34,  -0.77},
};
#endif

static constexpr uchar COLOR_CHART_MASK[24] = {
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0,
};

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "usage: " << argv[0] << " <input file> <output file>\n";
    return 1;
  }
  const char *input_file = argv[1];
  const char *output_file = argv[2];

  cv::Mat image = cv::imread(input_file, cv::IMREAD_COLOR);
  if (!image.data) {
    std::cerr << "Failed to read the input file.\n";
    return 1;
  }

  // Detect the color chart.
  cv::Mat imageCopy = image.clone();
  cv::Ptr<cv::mcc::DetectorParameters> params = cv::mcc::DetectorParameters::create();
  params->adaptiveThreshWinSizeMax = 500;
  cv::Ptr<cv::mcc::CCheckerDetector> detector = cv::mcc::CCheckerDetector::create();
  if (!detector->process(image, cv::mcc::MCC24, 1, false, params)) {
    std::cerr << "Failed to detect the color chart\n";
    return 2;
  }
  cv::Ptr<cv::mcc::CChecker> checker = detector->getBestColorChecker();
  if (DEBUG) cv::mcc::CCheckerDraw::create(checker)->draw(image);
  cv::Mat chartsRGB = checker->getChartsRGB();
  cv::Mat src = chartsRGB.col(1).clone().reshape(3, chartsRGB.rows / 3);
  src /= 255.0;

  // Compute the color correction matrix.
  cv::Mat colors(std::size(COLOR_CHART_LAB), 1, CV_64FC3);
  for (size_t i = 0; i < std::size(COLOR_CHART_LAB); i++) colors.at<cv::Vec3d>(i, 0) = cv::Vec3d(COLOR_CHART_LAB[i]);
  cv::Mat colored(std::size(COLOR_CHART_MASK), 1, CV_8U);
  for (size_t i = 0; i < std::size(COLOR_CHART_MASK); i++) colored.at<uchar>(i, 0) = COLOR_CHART_MASK[i];
  cv::ccm::ColorCorrectionModel model(src, colors, cv::ccm::COLOR_SPACE_Lab_D50_2, colored);
  model.setLinear(cv::ccm::LINEARIZATION_IDENTITY);  // For raw images without gamma correction.
  model.setCCM_TYPE(cv::ccm::CCM_4x3);
  model.run();
  cv::Mat ccm = model.getCCM();
  std::cout << "Loss: " << model.getLoss() << std::endl;
  std::cout << "CCM: "  << ccm  << std::endl;

  // Calibrate and save the image.
  cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
  image.convertTo(image, CV_64F);
  image = model.infer(image / 255.0) * 255.0;
  image.convertTo(image, CV_8UC3);
  cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
  cv::imwrite(output_file, image);

  return 0;
}
