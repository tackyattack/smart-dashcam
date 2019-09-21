all: apps

apps:
	$(MAKE) -C hello_triangle2
	$(MAKE) -C ogl_compute
	$(MAKE) -C ogl_accelerator
	$(MAKE) -C picam_tex

clean:
	$(MAKE) -C hello_triangle2 clean
	$(MAKE) -C ogl_compute clean
	$(MAKE) -C ogl_accelerator clean
	$(MAKE) -C picam_tex clean
