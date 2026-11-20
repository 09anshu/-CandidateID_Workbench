import fitz

doc = fitz.open("Heatsink_Thermal_Solver_Comparison.pdf")

def check_overlap(r1, r2):
    inter = r1 & r2
    return (inter.width > 5 and inter.height > 5)

print("--- Checking Image and Caption/Text Collisions ---")
img_collisions = 0
for page_idx, page in enumerate(doc):
    imgs = page.get_images()
    # Find rects of all images
    img_rects = []
    for info in page.get_image_info():
        img_rects.append(fitz.Rect(info["bbox"]))
    
    words = page.get_text("words")
    for r_img in img_rects:
        for w in words:
            r_w = fitz.Rect(w[:4])
            if check_overlap(r_img, r_w):
                print(f"Page {page_idx+1}: Image {r_img} intersects text '{w[4]}' at {r_w}")
                img_collisions += 1

print(f"Total image-text collisions across all 12 pages: {img_collisions}")
