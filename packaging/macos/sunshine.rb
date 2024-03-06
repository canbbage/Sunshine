require "language/node"

class @PROJECT_NAME@ < Formula
  desc "@PROJECT_DESCRIPTION@"
  homepage "@PROJECT_HOMEPAGE_URL@"
  url "@GITHUB_CLONE_URL@",
    tag: "@GITHUB_BRANCH@"
  version "@PROJECT_VERSION@"
  license all_of: ["GPL-3.0-only"]
  head "@GITHUB_CLONE_URL@"

  depends_on "boost" => :build
  depends_on "cmake" => :build
  depends_on "pkg-config" => :build
  depends_on "curl"
  depends_on "miniupnpc"
  depends_on "node"
  depends_on "openssl"
  depends_on "opus"

  def install
    # todo: fix vite build
    system "sed", "-i", "''", "s/SUNSHINE_SOURCE_ASSETS_DIR=${SUNSHINE_SOURCE_ASSETS_DIR} SUNSHINE_ASSETS_DIR=${CMAKE_BINARY_DIR}//", "cmake/targets/common.cmake"

    args = %W[
      -DBUIld_WERROR=ON
      -DCMAKE_INSTALL_PREFIX=#{prefix}
      -DOPENSSL_ROOT_DIR=#{Formula["openssl"].opt_prefix}
      -DSUNSHINE_ASSETS_DIR=sunshine/assets
    ]
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args, *args

    cd "build" do
      system "make", "-j"
      system "make", "install"
    end
  end

  service do
    run [opt_bin/"sunshine", "~/.config/sunshine/sunshine.conf"]
  end

  test do
    # test that the binary runs at all
    shell_output("#{bin}/sunshine --version").strip

    # todo: add unit tests
  end
end
