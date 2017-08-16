// (c) 2017 massanoori

#pragma once

/**
 * 東方のナンバリングを取得
 * 例：
 *   東方紅魔郷: 6.0
 *   東方文花帖: 9.5
 *   東方地霊殿: 11.0
 *   妖精大戦争: 12.8
 *
 * 東方の実行ファイル名は"th<番号><体験版ならtr>.exe"の形式、
 * もしくは".exe"であることを用いて取得する。
 * これらに当てはまらなければ、0.0を返す。
 */
double GetTouhouSeriesNumber();

/**
 * 実行ファイルをフルパスで取得する。
 */
boost::filesystem::path GetTouhouExecutableFilePath();

/**
 * 実行ファイルのファイル名を取得する。ディレクトリは含まない。
 */
boost::filesystem::path GetTouhouExecutableFilename();

/**
 * セーブデータを保存するディレクトリを取得する。
 */
boost::filesystem::path GetTouhouPlayerDataDirectory();

/**
 * スクリーンキャプチャのない東方であるならtrueを返す。
 *
 * 現在のところ、東方紅魔郷のみ。
 */
bool IsTouhouWithoutScreenCapture();
