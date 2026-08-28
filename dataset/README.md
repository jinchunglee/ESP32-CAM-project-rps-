
# 📁 Dataset Setup Instructions

To keep this repository lightweight, **raw image datasets are not included in this Git repository**. Please follow the instructions below to download or prepare the dataset before running the model training.

---

## 🔍 1. Dataset Sources

You can download a publicly available **Rock-Paper-Scissors** dataset from platforms like Kaggle or Roboflow, or use your own custom images:

* **Kaggle**: Search for `Rock Paper Scissors Dataset` (e.g., [Rock-Paper-Scissors Images](https://www.kaggle.com/datasets/drgfreeman/rockpaperscissors))
* **Roboflow**: Search for `Rock Paper Scissors Computer Vision Dataset`
* **Custom Photos**: Take your own photos for rock, paper, and scissors hand gestures (a simple background and uniform lighting are recommended).

---

## 🗂️ 2. Directory Structure

After downloading or preparing your images, create a `dataset/` folder in the project root directory and organize your files according to the structure below:

```text
dataset/
└── train/
    ├── rock/
    │   ├── img01.jpg
    │   ├── img02.jpg
    │   └── ...
    ├── paper/
    │   ├── img01.jpg
    │   ├── img02.jpg
    │   └── ...
    └── scissors/
        ├── img01.jpg
        ├── img02.jpg
        └── ...
