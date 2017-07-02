====================
開発
====================


THRotatorのソースコードの入手
==================================

THRotatorのソースコードは、 `GitHub <https://github.com/massanoori/THRotator>`_ から入手ができます。

とりあえずダウンロードして、ビルドしたり改造したりする場合は、
GitHubから直接ソースコードをダウンロードすることができます。
``Clone or download`` をクリックし、表示される ``Download ZIP`` をクリックするとダウンロードが始まります。
ダウンロードしたzipファイルを解凍すると、ソースコードが含まれたフォルダが作成されます。

お使いのPCにgitがインストール済みの場合は、
``Clone or download`` をクリックすると表示されるURLからクローンすることでも、
ソースコードを取得できます。


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

1. 使っているOSが64 bitの場合は、 ``スタートメニュー`` → ``Visual Studio 20XX`` → ``VS20XX x64 Native Tools コマンドプロンプト`` 、
   32 bitの場合は、 ``VS20XX x86 Native Tools コマンドプロンプト`` を開きます。
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

使っているOSが64 bitの場合は、プラットフォームはwin64-x64、win32-x86の両方を使えますが、
32 bitのOSの場合はwin32-x86しか実行できません。

.. _devel_proj_gen:


プロジェクトファイル生成
---------------------------

1. CMakeをインストーラからインストールした場合は、 ``スタートメニュー`` → ``CMake`` → ``CMake (cmake-gui)`` を実行します。
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

``docs-source/compile.bat`` を実行すると、すべての言語に対して、 ``docs/<言語>/_build`` 以下にhtml形式のドキュメントが生成されます。

言語ごとに生成したい場合は、
``docs-source/<言語>/make.bat html`` を実行してください。

GitHub Pagesとして公開されるディレクトリ ``docs/`` へコピーするには、 ``docs-source/update.bat`` を実行してください。


ローカライズ
=============

THRotatorはWindowsのMultilingual User Interface (MUI)による、
GUIやエラーメッセージの多言語化が可能です。

新しい言語を追加する際は、英語版のリソースをテンプレートとして容易に作成可能です。
フランス語を追加することを例に、作成の流れを見ていきましょう。

.. note:: ここでは、具体的な翻訳については扱いません。

1. テンプレートをコピー
-----------------------------

1. まず、ひな型となる ``localization_en-US`` があるフォルダに、
   新しく ``localization_<言語名>`` を作成します。
   言語名は、 `Available Language Packs for Windows <https://technet.microsoft.com/en-us/library/hh825678.aspx>`_ の **Language/culture name** です。
   また、このページには **Language hexadecimal identifier** に言語IDも記載されていますので、このIDもメモしておきます。
   今回はフランス語を例としていますので、 ``localization_fr-FR`` を作成します。
   また、言語IDの **0x040c** をメモしておきます。
2. ``localization_en-US`` の中にある、 ``CMakeLists.txt`` 、 ``resource.h`` 、 ``THRotator_en-US.rc`` を、
   新たに作成した ``localization_<言語名>`` (今回は ``localization_fr-FR``) にコピーします。
3. ``localization_<言語名>\THRotator_en-US.rc`` を、 ``localization_<言語名>\THRotator_<言語名>.rc`` (今回は ``THRotator_fr-FR.rc``)にリネームします。


2. CMakeの準備
--------------------------

1. ``localization_<言語名>\CMakeLists.txt`` を開き、 ``en-US`` となっている部分を、 ``<言語名>`` に置換します。
   また、 ``set(language_id 0x0409)`` の **0x0409** を、メモしておいた言語IDに置き換えます。
   今回は言語IDが **0x040c** なので、 ``set(language_id 0x040c)`` に変更します。
   ``localization_<言語名>\CMakeLists.txt`` の編集は以上です。
2. ソースコードのルートフォルダにある ``CMakeLists.txt`` を開き、

フランス語の例では、``localization_fr-FR\CMakeLists.txt`` の中身は次のようになります。 ::

    # resource language and its ID
    # for a list of languages and IDs, visit https://msdn.microsoft.com/en-us/library/hh825678.aspx
    set(language fr-FR) # 編集
    set(language_id 0x040c) # 編集

    include(../internationalization/THRotator_i18n.cmake)
	
また、ルートフォルダにある ``CMakeLists.txt`` の中身は次のようになります。 ::

    # 省略
	
    add_subdirectory(d3d9)
    add_subdirectory(d3d8)
    add_subdirectory(localization_en-US)
    add_subdirectory(localization_fr-FR) # 今回追加
	
3. プロジェクトファイルの生成
----------------------------------

本ページの :ref:`devel_proj_gen` で説明している方法で、プロジェクトファイルを生成します。

4. 翻訳
-----------------

``THRotator.sln`` を開きなおすか、再読み込みすると、
プロジェクト ``localization_<言語名>`` が追加されているはずです。

Visual Studioのリソースビューを開き、
``localization_<言語名>`` の中にあるString Tableやダイアログリソースを翻訳していきます。
String Tableやダイアログリソースのプロパティの ``Language`` が、 ``英語 (米国)`` になっていますので、
翻訳先の言語に変更してください。フランス語の例では、 ``フランス語 (フランス)`` に変更します。

5. ビルドと実行
----------------

プロジェクト ``localization_<言語名>`` をビルドすると、
``d3d8.dll`` 、 ``d3d9.dll`` の出力ディレクトリに言語名のフォルダができ、その中に ``.mui`` ファイルが出来上がります。

``d3d8.dll`` または ``d3d9.dll`` と一緒に、言語名のフォルダもゲームの実行ファイルのフォルダにコピーします。
そのままゲームを実行すると、お使いのPCにその言語が存在すれば、THRotatorのGUIやメッセージがその言語で表示されるようになります。


.. note::

   d3d8.dll.muiまたはd3d9.dll.muiには、それぞれd3d8.dll、d3d9.dllのチェックサムが埋め込まれます。
   .dllのチェックサムと.muiに埋め込まれたチェックサムが異なる場合は、言語の読み込みが失敗してしまいます。
