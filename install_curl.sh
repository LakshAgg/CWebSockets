# Detect package manager
if command -v apt-get &>/dev/null; then
    PACKAGE_MANAGER="apt-get"
elif command -v dnf &>/dev/null; then
    PACKAGE_MANAGER="dnf"
elif command -v yum &>/dev/null; then
    PACKAGE_MANAGER="yum"
elif command -v pacman &>/dev/null; then
    PACKAGE_MANAGER="pacman"
elif command -v zypper &>/dev/null; then
    PACKAGE_MANAGER="zypper"
else
    echo "Unsupported package manager"
    exit 1
fi

# Install required packages
case $PACKAGE_MANAGER in
    "apt-get")
        sudo $PACKAGE_MANAGER update;
        sudo $PACKAGE_MANAGER install -y curl wget unzip autoconf libtool libssl-dev;
        ;;
    "dnf"|"yum")
        sudo $PACKAGE_MANAGER install -y curl wget unzip autoconf libtool openssl-devel;
        ;;
    "pacman")
        sudo $PACKAGE_MANAGER -Sy --noconfirm curl wget unzip autoconf libtool openssl;
        ;;
    "zypper")
        sudo $PACKAGE_MANAGER install -y curl wget unzip autoconf libtool libopenssl-devel;
        ;;
esac

# get latest version tag
VERSION_CURL_LATEST=$(curl -s "https://api.github.com/repos/curl/curl/releases/latest" | grep -o '"tag_name": "[^"]*' | cut -d'"' -f4) && \

# make url
LOCATION_CURL_LATEST="https://github.com/curl/curl/archive/${VERSION_CURL_LATEST}.zip" && \

# fetch the zip file using wget
wget $LOCATION_CURL_LATEST -O "curl_latest.zip" && \

# unzip the fetched file and remove the zip
unzip curl_latest.zip && rm curl_latest.zip && \

# configure curl
cd "curl-${VERSION_CURL_LATEST}" && ./buildconf && ./configure --enable-websockets --with-openssl && \

# make
PROCCESSOR_COUNT=$(nproc) && make -j$PROCCESSOR_COUNT && \

# install curl
sudo make install && \
echo "export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH" >> ~/.bashrc && \
source ~/.bashrc

# rename installed library from libcurl to libcurlexp. This will help avoid conflicts while linking.
sudo mv /usr/local/lib/libcurl.so /usr/local/lib/libcurlexp.so && \

# clean
make clean && \

echo "libcurl with wss has been installed successfully :)" && \
echo "Use -lcurlexp while linking for this build." 