from maix import camera, display, app, time, image
import math

width = 512
height = 320

cam = camera.Camera(width, height)
disp = display.Display()

green_lab = [(88,92,-8,20,-17,0)]

final_img = image.Image(width, int(height/2))

while not app.need_exit():

    img = cam.read()
    final_img.clear()

    bin_img = img.copy()
    bin_img.binary(green_lab)

    blobs = bin_img.find_blobs([(100,255)], pixels_threshold=5, area_threshold=5)

    max_blob = None
    max_area = 0

    for b in blobs:

        x, y, w, h = b.rect()
        area = w * h

        if area > max_area:
            max_blob = b
            circularity = b.pixels() / area
            is_circle = circularity > 0.6
            if(is_circle):  # 只有是圆形才更新最大blob
                 max_area = area

        img.draw_rect(x, y, w, h, image.COLOR_BLUE)
        bin_img.draw_rect(x, y, w, h, image.COLOR_BLUE)

    roi_img = None

    # -------------------------
    # 最大blob圆形检测
    # -------------------------

    if max_blob:

        x, y, w, h = max_blob.rect()

        if is_circle:
            # 绿色框 = 圆形目标
            img.draw_rect(x, y, w, h, image.COLOR_GREEN, 2)
            bin_img.draw_rect(x, y, w, h, image.COLOR_GREEN, 2)

            roi_img = img.crop(x, y, w, h)
            roi_img.binary(green_lab)
            roi_img.erode(1)
            roi_img.dilate(1)

            final_img.draw_image(0, height//3, roi_img)

        else:

            # 红色框 = 非圆形
            img.draw_rect(x, y, w, h, image.COLOR_RED, 2)
            bin_img.draw_rect(x, y, w, h, image.COLOR_RED, 2)

    resize_img_raw = img.resize(width//3, height//3)
    resize_img_par = bin_img.resize(width//2, height//2)

    final_img.draw_image(0, 0, resize_img_raw)
    final_img.draw_image(width//3, 0, resize_img_par)

    disp.show(final_img)

    fps = time.fps()
    print(f"time: {1000/fps:.02f}ms, fps: {fps:.02f}")