CUDA Path Tracer
================

**University of Pennsylvania, CIS 565: GPU Programming and Architecture, Project 3**

* (TODO) YOUR NAME HERE
* Tested on: (TODO) Windows 22, i7-2222 @ 2.22GHz 22GB, GTX 222 222MB (Moore 2222 Lab)

### (TODO: Your README)

*DO NOT* leave the README to the last minute! It is a crucial part of the
project, and we will not be able to grade you without a good README.

Instructions (delete me)
========================

## Requirements

* Features:
  * Work-efficient stream compaction using shared memory
  * Raycasting from the camera into the scene through an imaginary grid of pixels
    * Casting to plane at distance based on FOV
  * Diffuse surfaces
    * Cosine weighted
  * (E) Non-perfect specular surfaces
    * Cosine weighted, restricted by specular exponent
    * Perfectly specular-reflective (mirrored) surfaces would be a non-perfect surface with very large (toward positive infinity) specular exponent
    * http://www.cs.cornell.edu/courses/cs4620/2012fa/lectures/37raytracing.pdf
  * (E) Refraction (ice / diamond) with Frensel effects using Schlick's approximation
    * https://en.wikipedia.org/wiki/Schlick's_approximation
  * (E) Subsurface scattering
    * Simplified version of Dipole
      * https://graphics.stanford.edu/papers/bssrdf/bssrdf.pdf
      * Reduced memory overhead by approximating ray-out position without passing geometry all the way into scatter function
      * Passing geometry slows the entire rendering process by a factor of ~3
    * SSS for reflective material (split on specular and diffuse, then further split on diffuse)
  * Antialiasing
    * Oversampling at each iteration
    * Effectively increases render time; proportional to # of oversampling passes
  * (E) Physically-based depth-of-field (by jittering rays within an aperture)
    * Using antialiasing routines but with different jittering method
    * Find focal plane
    * Jitter each ray on its origin
    * Keep ray end point intact so it always focuses on focal plane; equivalent to jittering camera itself around focal plane

* Scenes:
  * `cornell1`: mixed objects (specular, refraction, diffuse, caustic)
  * `cornell2`: SSS, same size spheres, mixed distances to light
  * `cornell3`: SSS + specular
  * `cornell4`: SSS, same size cubes, mixed distances to light
  * `cornell5`: mixed objects, closed scene, camera inside box
  * `cornell6`: single sphere, for performance testing

For each extra feature, you must provide the following analysis:

* Overview write-up of the feature
* Performance impact of the feature
* If you did something to accelerate the feature, what did you do and why?
* Compare your GPU version of the feature to a HYPOTHETICAL CPU version
  (you don't have to implement it!) Does it benefit or suffer from being
  implemented on the GPU?
* How might this feature be optimized beyond your current implementation?

## README

Please see: [**TIPS FOR WRITING AN AWESOME README**](https://github.com/pjcozzi/Articles/blob/master/CIS565/GitHubRepo/README.md)

* Sell your project.
* Assume the reader has a little knowledge of path tracing - don't go into
  detail explaining what it is. Focus on your project.
* Don't talk about it like it's an assignment - don't say what is and isn't
  "extra" or "extra credit." Talk about what you accomplished.
* Use this to document what you've done.
* *DO NOT* leave the README to the last minute! It is a crucial part of the
  project, and we will not be able to grade you without a good README.

In addition:

* This is a renderer, so include images that you've made!
* Be sure to back your claims for optimization with numbers and comparisons.
* If you reference any other material, please provide a link to it.
* You wil not be graded on how fast your path tracer runs, but getting close to
  real-time is always nice!
* If you have a fast GPU renderer, it is very good to show case this with a
  video to show interactivity. If you do so, please include a link!

### Analysis

* Stream compaction helps most after a few bounces. Print and plot the
  effects of stream compaction within a single iteration (i.e. the number of
  unterminated rays after each bounce) and evaluate the benefits you get from
  stream compaction.
```
800 * 800 cornell1
Depth: 0 / Grid size: 625125
Depth: 1 / Grid size: 497276
Depth: 2 / Grid size: 406024
Depth: 3 / Grid size: 326613
Depth: 4 / Grid size: 264707
Depth: 5 / Grid size: 214660
Depth: 6 / Grid size: 174592
Depth: 7 / Grid size: 141986
```

* Compare scenes which are open (like the given cornell box) and closed
  (i.e. no light can escape the scene). Again, compare the performance effects
  of stream compaction! Remember, stream compaction only affects rays which
  terminate, so what might you expect?
  * Open scene much faster. More rays got terminated due to rays shooting side ways are off to the ambient and thus no hits
  * Closed scene much slower. Less rays got terminated because all rays will hit at least a wall; theoretical 3x more slower with 3 passes; about 2x slower perceived. Closed scene is much brighter.

## Performance
* Baseline: `cornell6`, 200*200


## Submit

If you have modified any of the `CMakeLists.txt` files at all (aside from the
list of `SOURCE_FILES`), you must test that your project can build in Moore
100B/C. Beware of any build issues discussed on the Google Group.

1. Open a GitHub pull request so that we can see that you have finished.
   The title should be "Submission: YOUR NAME".
2. Send an email to the TA (gmail: kainino1+cis565@) with:
   * **Subject**: in the form of `[CIS565] Project N: PENNKEY`.
   * Direct link to your pull request on GitHub.
   * Estimate the amount of time you spent on the project.
   * If there were any outstanding problems, or if you did any extra
     work, *briefly* explain.
   * Feedback on the project itself, if any.
