====================
開発
====================


THRotatorのビルド
====================

ここでは、THRotatorのビルドするまでの手順を説明します。

ビルドに必要なソフトウェアは、次の通りです。

* Microsoft Visual C++ (2012以降)
* Boost C++ Libraries
* CMake

Microsoft Visual C++ のインストール
-----------------------------------------

.. note:: インストール済みの場合は不要です。

`Visual Studio のホームページ <https://www.visualstudio.com/>`_ から、Visual Studioをダウンロードしてください。
エディションは無料で利用可能なCommunityで問題ありません。

Visual Studio 2015以降の場合、Visual C++がデフォルトでインストールされないので、
インストールオプションでVisual C++をインストールするように設定を変更してください。

Boost C++ Libraries のインストール
-----------------------------------------

.. note:: ``filesystem`` モジュールがビルド済みの場合は不要です。

`Boost C++ Libraries <http://www.boost.org/>`_ の **Current Release** から最新版をダウンロードし、
適当なディレクトリに解凍します(例えば、 ``C:\boost\`` など)。

次に、boostの ``filesystem`` モジュールをビルドします。

1. 使っているOSが64 bitの場合は、「スタートメニュー」→「Visual Studio 20XX」→「VS20XX x64 Native Tools コマンドプロンプト」、
   32 bitの場合は、「VS20XX x86 Native Tools コマンドプロンプト」を開きます。
2. 開いたコマンドプロンプトで、解凍したboostの、 ``bootstrap.bat`` のあるフォルダへ移動します。
3. ``bootstrap.bat`` を実行します。
4. ``b2.exe`` が出来上がるので、そのあとに ``b2 --with-filesystem runtime-link=shared,static`` を実行します。

CMake のインストール
------------------------

.. note:: CMake 3.5 以降をインストール済みの場合は不要です。

`CMakeのホームページ <https://cmake.org/>`_ の **Download** にある、最新のリリース(Latest Release)から、以下のいずれかをダウンロードし、
インストーラの場合はインストールを行ってください。

* Windows win64-x64 Installer (64 bit OS向けインストーラ)
* Windows win64-x64 ZIP (64 bit OS向け実行ファイル)
* Windows win32-x86 Installer　(32 bit OS向けインストーラ)
* Windows win32-x86 ZIP (32 bit OS 向け実行ファイル)

Windows win64-x64 をインストールしてください。
使っているOSが64 bitの場合は、プラットフォームはwin64-x64、win32-x86の両方を使えますが、
32 bitのOSの場合はwin32-x86しか実行できません。


プロジェクトファイル生成
---------------------------

1. CMakeをインストーラからインストールした場合は、「スタートメニュー」→「CMake」→「CMake (cmake-gui)」を実行します。
   直接ダウンロードした場合は、 ``bin\cmake-gui.exe`` を実行します。
2. ``Where is the source code:`` に、THRotatorのソースコードがあるルートディレクトリのパスを入力します。
3. ``Where to build the binaries:`` に、プロジェクトファイルの生成先、およびビルドを行うディレクトリのパスを入力します。
4. ``Configure`` ボタンを押します。Generatorの選択画面が出るので、ここで ``Visual Studio XX 20XX`` を選択し、 ``Finish`` ボタンを押します。
   Win64、ARMが付いているものは選択しないでください。
5. ``BOOST_INCLUDE_DIR`` に、boostのインクルードディレクトリ(bootstrap.batがあるディレクトリ)のパスを入力します。
6. ``BOOST_LIB_DIR`` に、boostのライブラリディレクトリ (デフォルトでは、 ``<boostのインクルードディレクトリ>\stage\lib`` )を入力します。
7. 再度 ``Configure`` ボタンを押し、エラーがなければ ``Generate`` ボタンを押します。


ビルド
---------------------------------

1. プロジェクト生成先にある ``THRotator.sln`` を開いてください。
2. 5つのプロジェクトが読み込まれます。
 
  * ALL_BUILD (すべてのプロジェクトをビルドするプロジェクト)
  * d3d8 (Direct3D 8版THRotator)
  * d3d9 (Direct3D 9版THRotator)
  * localization_en-US (THRotatorの英語UIリソースDLL)
  * ZERO_CHECK (CMakeが自動で生成)

3. Direct3D 8版をビルドしたい場合は ``d3d8`` 、Direct3D 9版をビルドしたい場合は ``d3d9`` をビルドしてください。


ドキュメントのビルド
====================

ドキュメントのビルドは、Sphinxを用いています。

Sphinxのインストール
------------------------

http://sphinx-users.jp/gettingstarted/install_windows.html をご覧ください。

Sphinxテーマのインストール
------------------------------

THRotatorでは、テーマとして `sphinx_rtd_theme <https://github.com/rtfd/sphinx_rtd_theme>`_ を使っています。
このテーマのインストールは、 ``python -m pip install sphinx_rtd_theme`` を実行することでインストールできます。

ドキュメントのビルド
----------------------------

``doc/<言語>/make.bat html`` を実行すると、 ``doc/<言語>/_build`` 以下にhtml形式のドキュメントが生成されます。


ローカライズ
=============

THRotatorはWindowsのMultilingual User Interface (MUI)による多言語対応が可能です。


