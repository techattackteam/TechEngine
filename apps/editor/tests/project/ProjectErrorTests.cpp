#include <TechEngine/testing/ErrorCategoryChecks.hpp>

#include <project/ProjectError.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>

using TechEngine::ProjectError;

static constexpr std::array<ProjectError, 15> ALL_CODES{
    ProjectError::ParseFailed,
    ProjectError::SchemaInvalid,
    ProjectError::NoStartupScene,
    ProjectError::UnknownStartupScene,
    ProjectError::StartupSceneNotInTarget,
    ProjectError::UnknownReference,
    ProjectError::SourceUnreadable,
    ProjectError::SourceHasNoMeshes,
    ProjectError::ImportFolderExists,
    ProjectError::SourceCompressed,
    ProjectError::InvalidImportScale,
    ProjectError::UnsupportedImageType,
    ProjectError::SourceNotInAssets,
    ProjectError::ImageUnreadable,
    ProjectError::ImportOutputExists,
};

TEST_CASE("every ProjectError code converts into the project category", "[editor][project][projecterror]") {
    TechEngineTests::checkErrorCategory<ProjectError>(ALL_CODES, TechEngine::projectErrorCategory(), "project");
}
