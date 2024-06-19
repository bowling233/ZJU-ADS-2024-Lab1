# 基于自适应基数树的高性能文本倒排索引

浙江大学《高级数据结构与算法》课程实验一：Roll Your Own Mini Search Engine。

- Presentation：见 [presentation-masked.pdf](./presentation-masked.pdf)
- 报告：见 [report-masked.pdf](report-masked.pdf)

本实验采用 ART 树实现，论文见 [The adaptive radix tree: ARTful indexing for main-memory databases](https://doi.org/10.1109/ICDE.2013.6544812)，这是受 [Typesense](https://github.com/typesense/typesense) 启发。具体使用的 C++ ART 实现为 [rafaelkallis/adaptive-radix-tree/](https://github.com/rafaelkallis/adaptive-radix-tree/)，他在仓库中页给出了比较详细的分析报告。本实验也使用了 [smassung/porter2_stemmer](https://github.com/smassung/porter2_stemmer) 作为 Stemmer。

## 程序编译

请阅读 Makefile，在 Linux/macOS 下使用 `make` 编译运行。

## 输入输出格式

### 输入格式

程序具有以下指令：

- `Read DocList <listfile>`：读取文件列表中的所有文件。
- `Read StopWord <stopfile>`：读取停用词列表。
- `Read Doc <file>`：读取一个文件。
- `Query Word <word>`：查询一个单词。
- `Query List <word>`：查询一个单词列表。
- `Show Info <file>`：查询索引数据库状态。
- `Show Term <file>`：查询所有单词。

## 实验要求：Roll Your Own Mini Search Engine

In this project, you are supposed to create your own mini search engine which can handle inquiries over “The Complete Works of William Shakespeare” (<http://shakespeare.mit.edu/>).

You may download the functions for handling stop words and stemming from the Internet, as long as you add the source in your reference list.

Your tasks are:

- 1 Run a word count over the Shakespeare set and try to identify the stop words (also called the noisy words) – How and where do you draw the line between “interesting” and “noisy” words?
- 2 Create your inverted index over the Shakespeare set with word stemming. The stop words identified in part (1) must not be included.
- 3 Write a query program on top of your inverted file index, which will accept a user-specified word (or phrase) and return the IDs of the documents that contain that word.
- 4 Run tests to show how the thresholds on query may affect the results.

### Grading Policy

Programming: Write the programs for word counting (1 pt.), index generation (5 pts.) and query processing (3 pts.) with sufficient comments.

Testing: Design tests for the correctness of the inverted index (2 pts.) and thresholding for query (2 pts.). Write analysis and comments (3 pts.). Bonus: What if you have 500 000 files and 400 000 000 distinct words? Will your program still work? (+2 pts.)

Documentation: Chapter 1 (1 pt.), Chapter 2 (2 pts.), and finally a complete report (1 point for overall style of documentation).

Note: Anyone who does excellent job on answering the Bonus question will gain extra points.