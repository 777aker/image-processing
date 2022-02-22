// do nothing
#version 120

uniform float dX;
uniform float dY;
uniform float mode;
uniform sampler2D img;
uniform sampler2D img1;
uniform float shadowmult;

vec4 sample(float dx,float dy) {
    return texture2D(img,gl_TexCoord[0].st+vec2(dx,dy));
}

vec4 sample1(float dx,float dy) {
    return texture2D(img1,gl_TexCoord[0].st+vec2(dx,dy));
}

vec4 samp1(float dx, float dy) {
	return texture2D(img1,gl_TexCoord[0].st+vec2(dx*dX,dy*dY));
}

vec4 samp(float dx, float dy) {
	return texture2D(img,gl_TexCoord[0].st+vec2(dx*dX,dy*dY));	
}

void main() {
	// this mode just does sobel edge detection
	if(mode == 0) {
		vec4 H = -sample(-dX,+dY) - 2.0*sample(0.0,+dY) - sample(+dX,+dY)
				 +sample(-dX,-dY) + 2.0*sample(0.0,-dY) + sample(+dX,-dY);

		vec4 V =      sample(-dX,+dY)  -     sample(+dX,+dY)
				+ 2.0*sample(-dX,0.0)  - 2.0*sample(+dX,0.0)
				+     sample(-dX,-dY)  -     sample(+dX,-dY);

		gl_FragColor = sqrt(H*H+V*V);
	// this mode takes the result of sobel and subtracts
	// it from our image
	} else if(mode == 1) {
		vec4 pixel = texture2D(img,gl_TexCoord[0].st);
		vec4 pixel1 = texture2D(img1,gl_TexCoord[0].st);
		gl_FragColor = pixel - pixel1;
	// this mode takes the result of sobel and adds it
	} else if(mode==2) {
		vec4 pixel = texture2D(img,gl_TexCoord[0].st);
		vec4 pixel1 = texture2D(img1,gl_TexCoord[0].st);
		gl_FragColor = pixel + pixel1;
	// this method makes a weird vintage effect by accident
	} else if(mode==3) {
		vec4 avg = {0,0,0,0};
		for(float i = -1; i < 2; i++) {
			for(float j = -1; j < 2; j++) {
				avg += clamp(sample(i,j) - sample1(i,j), 0, 1)/9;
			}
		}
		gl_FragColor = avg;
	// now what I set out to do originally (cartoon shader hopefully)
	// this is going to take the average of all the pixels around it
	// but if it hits an edge it's going to discard those
	// so hopefully it will average pixels together that are not
	// outside of an edge
	} else if(mode == 4) {
		vec4 avg = {0,0,0,0};
		vec4 leftsum = {0,0,0,0};
		vec4 rightsum = {0,0,0,0};
		float size = 2;
		float discardsize = 0.5;
		for(float y = -size; y <= size; y++) {
			for(float x = -size; x < 0; x++) {
				if(length(samp1(x,y)) <= discardsize)
					leftsum += samp(x,y);
				else
					leftsum -= leftsum;
			}
			for(float x = size; x > 0; x--) {
				if(length(samp1(x,y)) <= discardsize)
					rightsum += samp(x,y);
				else
					rightsum -= rightsum;
			}
			if(length(samp1(0,y)) <= discardsize);
				avg += leftsum + rightsum + samp(0,y);
		}
		avg /= length(avg);
		gl_FragColor = avg;
	// this pass averages pixels from original image and our new image
	} else if(mode == 5) {
		vec4 pixel = texture2D(img,gl_TexCoord[0].st);
		vec4 pixel1 = texture2D(img1,gl_TexCoord[0].st);
		gl_FragColor = (pixel - pixel1);
	} else if(mode == 6) {
		vec4 pixel = texture2D(img,gl_TexCoord[0].st);
		vec4 pixel1 = texture2D(img1,gl_TexCoord[0].st);
		// this creates a strange shadow map effect
		if(pixel.x > pixel.y && pixel.x > pixel.z)
			pixel.x *= shadowmult;
		if(pixel.y > pixel.x && pixel.y > pixel.z)
			pixel.y *= shadowmult;
		if(pixel.z > pixel.x && pixel.z > pixel.x)
			pixel.z *= shadowmult;
		else
			pixel *= 1;
		// it's a little too dark so add a vec4 to make brighter
		gl_FragColor = (pixel - pixel1) + vec4(.1,.1,.1,.1);
	}
}