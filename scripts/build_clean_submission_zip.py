import os
import zipfile
import shutil
import fitz

root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
zip_path = os.path.join(root_dir, "CandidateID_Workbench.zip")

print("=================================================================")
print("STRICT SUBMISSION ZIP BUILDER — PASS SCIENCE WORKS WORKBENCH")
print("=================================================================")

# 1. Regenerate optimized PDFs
print("\n1. Verifying & regenerating required PDFs...")
import subprocess
subprocess.run(["python", os.path.join(root_dir, "scripts", "generate_pdfs.py")], check=True)

report_pdf = os.path.join(root_dir, "docs", "technical_report.pdf")
appendix_pdf = os.path.join(root_dir, "docs", "handwritten_appendix.pdf")

doc_r = fitz.open(report_pdf)
doc_a = fitz.open(appendix_pdf)
print(f"  • technical_report.pdf: {len(doc_r)} pages (Required: <= 6) -> PASSED")
print(f"  • handwritten_appendix.pdf: {len(doc_a)} pages (Required: <= 2) -> PASSED")
assert len(doc_r) <= 6, f"Technical report exceeds 6 pages: {len(doc_r)}"
assert len(doc_a) <= 2, f"Handwritten appendix exceeds 2 pages: {len(doc_a)}"

# 2. Strict file inclusion manifest
ALLOWED_MARKDOWNS = {
    "README.md",
    "docs/ai_usage_log.md",
    "docs/ownership_statement.md"
}

files_to_pack = []

def add_file(rel_path):
    full_path = os.path.join(root_dir, rel_path)
    if not os.path.exists(full_path):
        raise FileNotFoundError(f"Required file missing: {rel_path}")
    
    # Check markdown restriction
    if rel_path.lower().endswith(".md"):
        norm_rel = rel_path.replace("\\", "/")
        if norm_rel not in ALLOWED_MARKDOWNS:
            raise ValueError(f"FORBIDDEN MARKDOWN FILE REJECTED: {rel_path}")
            
    files_to_pack.append(rel_path)

def add_dir(rel_dir, exclude_exts=None, exclude_names=None):
    if exclude_exts is None: exclude_exts = set()
    if exclude_names is None: exclude_names = set()
    
    full_dir = os.path.join(root_dir, rel_dir)
    for root, dirs, files in os.walk(full_dir):
        # Prune excluded directories
        dirs[:] = [d for d in dirs if d not in {"node_modules", ".next", "build", "__pycache__", ".git", "staging_pkg"}]
        for f in files:
            ext = os.path.splitext(f)[1].lower()
            if ext in exclude_exts:
                continue
            if f in exclude_names:
                continue
            rel_f = os.path.relpath(os.path.join(root, f), root_dir).replace("\\", "/")
            if ext == ".md" and rel_f not in ALLOWED_MARKDOWNS:
                # Silently skip non-permitted markdown files
                continue
            files_to_pack.append(rel_f)

print("\n2. Assembling strict submission manifest...")

# Root executables & configuration
add_file("Launch.exe")
add_file("workbench_backend.exe")
add_file("workbench_tests.exe")
add_file("libgcc_s_seh-1.dll")
add_file("libstdc++-6.dll")
add_file("libwinpthread-1.dll")
add_file("CMakeLists.txt")
add_file("README.md")
add_file(".vscode/launch.json")
add_file(".vscode/tasks.json")

# Bundled UI
add_dir("ui")

# Data
add_file("data/geometry/sample_cube.stl")
add_file("data/geometry/provenance.txt")
add_file("data/thermal/sample_temperature.csv")
add_file("data/thermal/provenance.txt")

# Results (CSVs and PNG plots)
add_dir("results", exclude_exts={".md"})

# Docs (Only the approved PDFs, text walkthrough, and the 2 permitted markdowns)
add_file("docs/technical_report.pdf")
add_file("docs/handwritten_appendix.pdf")
add_file("docs/ai_usage_log.md")
add_file("docs/ownership_statement.md")
add_file("docs/demonstration_walkthrough.txt")

# Version control evidence
add_file("version_control/CandidateID_Workbench.bundle")
add_file("version_control/commit_log.txt")

# Tests
add_file("tests/test_main.cpp")

# Source Code
add_dir("src/core", exclude_exts={".md"})
add_dir("src/geometry", exclude_exts={".md"})
add_dir("src/thermal", exclude_exts={".md"})
add_dir("src/performance", exclude_exts={".md"})
add_dir("src/hardware", exclude_exts={".md"})
add_dir("src/backpropagation", exclude_exts={".md"})
add_dir("src/backend", exclude_exts={".md"})
# Frontend source code (excluding node_modules and .next)
add_file("src/frontend/package.json")
add_file("src/frontend/package-lock.json")
add_file("src/frontend/tsconfig.json")
add_file("src/frontend/tailwind.config.ts")
add_file("src/frontend/postcss.config.js")
add_file("src/frontend/next.config.js")
add_file("src/frontend/next-env.d.ts")
add_dir("src/frontend/app", exclude_exts={".md"})
add_dir("src/frontend/components", exclude_exts={".md"})
add_dir("src/frontend/lib", exclude_exts={".md"})

# Deduplicate
files_to_pack = sorted(list(set(files_to_pack)))

# 3. Audit markdown files in manifest
md_in_manifest = [f for f in files_to_pack if f.lower().endswith(".md")]
print(f"\n3. Auditing Markdown files in submission manifest:")
for m in md_in_manifest:
    print(f"   [PERMITTED MD] {m}")

if set(md_in_manifest) != ALLOWED_MARKDOWNS:
    raise RuntimeError(f"Markdown policy violation! Expected {ALLOWED_MARKDOWNS}, got {set(md_in_manifest)}")
print(f"   --> EXACTLY {len(md_in_manifest)} MARKDOWN FILES FOUND. POLICY SATISFIED!")

# 4. Create ZIP
if os.path.exists(zip_path):
    os.remove(zip_path)

print(f"\n4. Packaging {len(files_to_pack)} verified entries into {zip_path}...")
with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as zf:
    for f in files_to_pack:
        full_p = os.path.join(root_dir, f)
        zf.write(full_p, arcname=f)

zip_size_mb = os.path.getsize(zip_path) / (1024 * 1024)
print(f"   Successfully generated CandidateID_Workbench.zip ({zip_size_mb:.2f} MB)")

# 5. Independent Clean Extraction Audit
clean_audit_dir = os.path.join(root_dir, "clean_final_audit")
if os.path.exists(clean_audit_dir):
    shutil.rmtree(clean_audit_dir)
os.makedirs(clean_audit_dir)

print(f"\n5. Extracting into clean test folder: {clean_audit_dir}...")
with zipfile.ZipFile(zip_path, 'r') as zf:
    zf.extractall(clean_audit_dir)
    extracted_names = zf.namelist()

# Verify extracted content
extracted_mds = [f for f in extracted_names if f.lower().endswith(".md")]
print(f"   • Extracted total items: {len(extracted_names)}")
print(f"   • Extracted markdown files: {extracted_mds}")
assert set(extracted_mds) == ALLOWED_MARKDOWNS, "Extracted archive contains invalid markdown files!"
assert "Launch.exe" in extracted_names, "Launch.exe missing from ZIP!"
assert "workbench_backend.exe" in extracted_names, "workbench_backend.exe missing from ZIP!"
assert "workbench_tests.exe" in extracted_names, "workbench_tests.exe missing from ZIP!"
assert "docs/technical_report.pdf" in extracted_names, "technical_report.pdf missing from ZIP!"
assert "docs/handwritten_appendix.pdf" in extracted_names, "handwritten_appendix.pdf missing from ZIP!"
assert "version_control/CandidateID_Workbench.bundle" in extracted_names, "Git bundle missing from ZIP!"

print("\n6. Running test suite inside extracted clean directory...")
test_exe = os.path.join(clean_audit_dir, "workbench_tests.exe")
res = subprocess.run([test_exe], cwd=clean_audit_dir, capture_output=True, text=True)
if res.returncode == 0 and "43 / 43 PASSED" in res.stdout:
    print("   --> 43 / 43 TESTS PASSED INSIDE CLEAN EXTRACTED PACKAGE!")
else:
    print(res.stdout)
    print(res.stderr)
    raise RuntimeError("Test execution failed inside clean extracted directory!")

# Cleanup audit directory
shutil.rmtree(clean_audit_dir)
print("\n=================================================================")
print("FINAL AUDIT RESULT: 100% PASSED — ARCHIVE READY FOR SUBMISSION!")
print("=================================================================")
