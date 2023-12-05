# rotImage

simple command line tool to rotate an image.. optionally tries to detect the rotation angle needed to straighten the iumage, needs an image with obvious horizontal lines.

To rotate an image by a specific angle:
  -  ./rotImage -i <input_path> -o <output_path> -a 90
  -  ./rotImage -i <input_filename> -o <output_filename> -a 90
  
# build

open sln in MSVC with vcpkg integrated and build it.

# usage

## Arguments

- `-i`, `--input`: Specify the input image file path or input directory path.
  - This argument is required.

- `-o`, `--output`: Specify the output image file path or output directory path.
  - This argument is required.

- `-a`, `--angle`: Specify the rotation angle in degrees.
  - Accepts a double value.
  - Default value is `0.0`.

- `-r`, `--recursive`: Recursively process all image files in subdirectories.
  - Default value is `false`.
  - Implicit value when used is `true`.

- `-v`, `--verbose`: Enable verbose output.
  - Default value is `false`.
  - Implicit value when used is `true`.

- `-d`, `--detect`: Automatically detect and correct the rotation angle of the image.
  - Default value is `false`.
  - Implicit value when used is `true`.

- `-ref`, `--reference`: Specify the path to a reference image. The approximated rotation angle of this image will be used for all other images.

