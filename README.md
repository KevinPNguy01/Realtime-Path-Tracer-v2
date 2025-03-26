Software Ray Tracer v2

Overview

This project is a software-based ray tracer that utilizes the Windows API to display a window, meaning it is only compatible with Windows.

How to Run

Option 1: Run from Visual Studio

Open Software-Ray-Tracer-v2.sln in Visual Studio.

Click Run or build the project.

Option 2: Run the Prebuilt Executable

Navigate to the directory containing Software-Ray-Tracer-v2.exe.

Run Software-Ray-Tracer-v2.exe.

Dependencies

OpenImageDenoise (OIDN)

This project uses Intel Open Image Denoise (OIDN) for denoising. To ensure compatibility, your CPU must support:

SSE4.1 (Minimum requirement)

AVX2 (Recommended for better performance)

AVX-512 (For best performance on supported Intel CPUs)

To check if your CPU supports these instructions:

Windows: Open Command Prompt and run:

wmic cpu get Name

Then, search for your CPU model online to verify instruction set support.

Linux/macOS: Open Terminal and run:

cat /proc/cpuinfo | grep -m 1 "flags"

Look for sse4_1, avx2, or avx512*.

Notes

This project does not support Linux or macOS due to its dependency on the Windows API.