#!/bin/bash
# Script to convert Markdown documentation to PDF using pandoc

# Check if pandoc is installed
if ! command -v pandoc &> /dev/null; then
    echo "Pandoc is not installed. Installing..."
    sudo apt-get update
    sudo apt-get install -y pandoc texlive-latex-base texlive-fonts-recommended texlive-latex-extra
fi

# Convert markdown to PDF
pandoc doc/PROJECT_DOCUMENTATION.md \
    -o doc/PROJECT_DOCUMENTATION.pdf \
    --toc \
    --toc-depth=3 \
    --number-sections \
    --highlight-style=tango \
    --pdf-engine=pdflatex \
    -V geometry:margin=1in \
    -V documentclass=report \
    -V fontsize=11pt \
    -V linkcolor=blue \
    --metadata title="Circuit Simulator 3 - Project Documentation" \
    --metadata author="Team: Aykhan, Nurgül, Rafin, Sayad, Thanh" \
    --metadata date="December 2025"

echo "PDF generated: doc/PROJECT_DOCUMENTATION.pdf"
