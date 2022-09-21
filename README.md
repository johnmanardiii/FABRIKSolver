# FABRIKSolver
![GIF of FABRIK Solver](https://github.com/johnmanardiii/FABRIKSolver/blob/main/images/fabrik.gif?raw=true)

Done as a project for a computer animation class at Cal Poly, this project demonstrates a simple impmlementation of FABRIK (forwards and backwards inverse kinematics).

The arm is divided into three segments of a specific length and then calculates 3 final positions for each arm segment based on a randomly selected target (one of the white spheres) using FABRIK. Once it calculates a final valid set of positions for each of the different points on the arm, the arm animates to that final position. Animation is done by slerping between the initial forward direction of the arm segment to the final forward direction of the arm segment, calculated using the difference of the start and end point of each arm. Each arm segment (line) is then placed under the constraint that it starts where the previous arm segment ended.

For orientations, I represented the initial and final orientations of each segment of the arm as both matrices and quaternions. After calcuating the final position using FABRIK, I then created an orthonormal basis for the final orientation using the [Gram–Schmidt process](https://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process). I assumed the "up" vector for this matrix was world up (0, 1, 0), unless the forward vector was already aligned with the up vector, where I randomly generated vectors until I found one that wasn't colinear with the world up vector. From there, I used glm's constructor to create a quaternion representation of both the starting and final orientations of each arm segment. These quaternions were stored over the course of the whole animation and were spherically interpolated to produce the result shown in the GIF above.

Additionally, I brought in a bloom implementation I use a lot for a slight glow around the objects.
