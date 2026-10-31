import os
import zipfile
import shutil

root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
zip_path = os.path.join(root_dir, "CandidateID_Workbench.zip")

print(f"Creating submission package: {zip_path}")

if os.path.exists(zip_path):
    os.remove(zip_path)

include_items = [
    "Launch.exe",
    "workbench_backend.exe",
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll",
    "CMakeLists.txt",
    "README.md",
    "AGENTS.md",
    "GEMINI.md",
    "CLAUDE.md",
    "SKILL.md",
    "tasks.md",
    "task_plan.md",
    "findings.md",
    "progress.md",
    ".gitignore",
    ".vscode",
    "data",
    "docs",
    "results",
    "scripts",
    "src",
    "tests",
    "ui"
]

exclude_dirs = {
    "node_modules",
    ".next",
    "build",
    "staging_pkg",
    "__pycache__"
}

with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as zipf:
    for item in include_items:
        item_path = os.path.join(root_dir, item)
        if not os.path.exists(item_path):
            continue

        if os.path.isfile(item_path):
            zipf.write(item_path, arcname=item)
        elif os.path.isdir(item_path):
            for dirpath, dirnames, filenames in os.walk(item_path):
                # Filter out excluded directories
                dirnames[:] = [d for d in dirnames if d not in exclude_dirs]
                for file in filenames:
                    if file.endswith((".pyc", ".tmp", ".log")):
                        continue
                    full_file = os.path.join(dirpath, file)
                    rel_arc = os.path.relpath(full_file, root_dir)
                    zipf.write(full_file, arcname=rel_arc)

zip_size_mb = os.path.getsize(zip_path) / (1024 * 1024)
print(f"Successfully packaged CandidateID_Workbench.zip ({zip_size_mb:.2f} MB)")
