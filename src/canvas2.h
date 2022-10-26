// Canvas2 Checklist:
//  𐌢 [BLOCKING] Preprocessor for shaders (#include)
//	𐌢 Cleanup existing code
//  𐌢 Abstract from SDL, maybe? 
//		|-> Is this a good idea?
//  𐌢 Quadratic stroke interpolation (lookahead one frame)
//		|-> Involves solving a quadratic equation on the GPU, should be ok
//		|-> Is adding an extra frame of lag unacceptable?
//  𐌢 Separate from image size
//		|-> Use scrollwheel to zoom in/out, shift-mclick to pan
//	𐌢 Extend paintbrush
//		|-> Parametrize by outer radius (at 0) inner radius (at 1) & hardness (degree of polynomial)
//		|-> Note that all these can be implemented on layers on top of the fundamental SDF
//	𐌢 Accumulate vs Continuous mode
//		|-> continuous mode does "perfect interpolation" along stroke, doesn't commit stroke until mouse up
//		|-> accumulate mode does the basic "spit out a bunch of circles in a loop"
//	𐌢 Allow brush textures
//		|-> Will force accumulate mode, unsure how one would do perfect interpolation with a brush texture
//		|-> For loop in the compute shader? Cache texture reads within workgroup?
//	𐌢 Blur brush
//		|-> GPU convolution
//		|-> For radius: instead of lerping with unblurred background, as is the norm, lerp the KERNELS
//		|-> This way we get a kinda smooth falloff
//  𐌢 Change cursor to "hitmarker" when not over UI element?
//  𐌢 Somehow dock brush stroke UI to the side of the screen
//  𐌢 More keyboard shortcuts!!!