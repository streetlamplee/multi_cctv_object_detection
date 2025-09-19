#pragma once

#include <filesystem>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>


int count_file(const std::filesystem::path& directoryPath, std::string extension) {
    if (!std::filesystem::exists(directoryPath) || !std::filesystem::is_directory(directoryPath)) {
        std::cerr << "directory not found" << std::endl;
        return -1;
    }

    int count = 0;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                std::string exts = entry.path().extension().string();
                if (exts == extension) {
                    count++;
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "파일 시스템 오류 발생" << std::endl;
        return -1;
    }

    return count;
}

std::filesystem::path find_oldest_jpg_file(const std::filesystem::path& directoryPath, std::string extension) {
    // 디렉토리가 존재하는지 확인
    if (!std::filesystem::exists(directoryPath) || !std::filesystem::is_directory(directoryPath)) {
        std::cerr << "오류: '" << directoryPath.string() << "' 디렉토리를 찾을 수 없거나 디렉토리가 아닙니다." << std::endl;
        return std::filesystem::path(); // 비어 있는 경로 반환
    }

    std::filesystem::path oldest_file_path;
    // file_time_type의 최대값으로 초기화하여 어떤 파일이든 이보다 오래되었도록 설정
    auto oldest_time = std::filesystem::file_time_type::max();

    try {
        // 디렉토리 내의 모든 항목을 순회
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            // 항목이 일반 파일이고, 확장자가 .jpg 또는 .JPG인지 확인
            if (entry.is_regular_file()) {
                std::string exts = entry.path().extension().string();
                if (exts == extension || exts == extension) {
                    // 현재 파일의 마지막 수정 시간을 가져옴
                    auto current_time = std::filesystem::last_write_time(entry);

                    // 현재까지 가장 오래된 파일보다 더 오래되었으면, 정보를 갱신
                    if (current_time < oldest_time) {
                        oldest_time = current_time;
                        oldest_file_path = entry.path();
                    }
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "파일 시스템 오류 발생: " << e.what() << std::endl;
        return std::filesystem::path(); // 오류 발생 시 비어 있는 경로 반환
    }

    return oldest_file_path;
}

/**
 * @brief 디렉토리의 전체 파일 수가 임계값(threshold)을 초과하면,
 * 초과된 개수만큼 가장 오래된 파일부터 순서대로 삭제합니다.
 *
 * @param directoryPath 확인할 디렉토리의 경로.
 * @param threshold 유지할 최대 파일 개수.
 * @return int 삭제된 파일의 개수.
 */
int delete_oldest_file_threshold(const std::filesystem::path& directoryPath, size_t threshold) {
    // 1. 디렉토리 유효성 검사
    if (!std::filesystem::exists(directoryPath) || !std::filesystem::is_directory(directoryPath)) {
        std::cerr << "오류: '" << directoryPath.string() << "' 디렉토리를 찾을 수 없습니다." << std::endl;
        return 0;
    }

    std::vector<std::pair<std::filesystem::path, std::filesystem::file_time_type>> files;
    int deleted_count = 0;

    try {
        // 2. 디렉토리를 순회하며 모든 파일의 정보 수집
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                files.push_back({entry.path(), std::filesystem::last_write_time(entry)});
            }
        }

        // 3. 파일 개수가 임계값(threshold) 이하이면 작업 불필요
        if (files.size() <= threshold) {
            // std::cout << "파일 개수(" << files.size() << ")가 임계값(" << threshold << ") 이하이므로 작업을 수행하지 않습니다." << std::endl;
            return 0;
        }

        size_t num_to_delete = files.size() - threshold;
        // std::cout << "파일 개수(" << files.size() << ")가 임계값(" << threshold << ")을 초과하여, 가장 오래된 " << num_to_delete << "개의 파일을 삭제합니다." << std::endl;

        // 4. 파일을 수정 시간 순으로 정렬 (오래된 순 -> 최신 순)
        std::sort(files.begin(), files.end(),
            [](const auto& a, const auto& b) {
                return a.second < b.second; // 시간을 기준으로 오름차순 정렬
            }
        );

        // 5. 삭제할 개수만큼 가장 오래된 파일부터 순서대로 삭제
        for (size_t i = 0; i < num_to_delete; ++i) {
            try {
                const std::filesystem::path& path_to_delete = files[i].first;
                // std::cout << "  - 삭제 중: " << path_to_delete.string() << std::endl;
                std::filesystem::remove(path_to_delete);
                deleted_count++;
            } catch (const std::filesystem::filesystem_error& e) {
                // 개별 파일 삭제 실패 시 오류 메시지만 출력하고 계속 진행
                std::cerr << "  - 파일 삭제 오류: " << e.what() << std::endl;
            }
        }

    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "파일 시스템 오류 발생: " << e.what() << std::endl;
        return 0;
    }

    return deleted_count;
}