// Copyright (c) 2014-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>
#include "camera.h"

const std::string subsystem = "libv4l2-camera-plugin.so";
const std::string devname = "/dev/video0";

class SDKTest : public testing::Test
{
public:
    Camera *camera_;
    virtual void SetUp()
    {
        camera_ = new Camera();
    }
    virtual void TearDown()
    {
        delete camera_;
    }
};

TEST_F(SDKTest, Init)
{
    int retval = camera_->init(subsystem);
    EXPECT_EQ(CAMERA_ERROR_NONE, retval);
    EXPECT_EQ(CAMERA_STATE_INIT, camera_->getCameraState());
    retval = camera_->deinit();
}

TEST_F(SDKTest, DeInit)
{
    int retval = camera_->init(subsystem);
    retval = camera_->deinit();
    EXPECT_EQ(CAMERA_ERROR_NONE, retval);
    EXPECT_EQ(CAMERA_STATE_UNKNOWN, camera_->getCameraState());
}

TEST_F(SDKTest, Open)
{
    int retval = camera_->init(subsystem);

    retval = camera_->open(devname);
    EXPECT_EQ(CAMERA_ERROR_NONE, retval);
    EXPECT_EQ(CAMERA_STATE_OPEN, camera_->getCameraState());

    retval = camera_->open(devname);
    EXPECT_EQ(CAMERA_ERROR_DEVICE_OPEN, retval);
    EXPECT_EQ(CAMERA_STATE_OPEN, camera_->getCameraState());

    retval = camera_->close();
    retval = camera_->deinit();
}

TEST_F(SDKTest, Close)
{
    int retval = camera_->init(subsystem);
    retval = camera_->open(devname);

    retval = camera_->close();
    EXPECT_EQ(CAMERA_ERROR_NONE, retval);
    EXPECT_EQ(CAMERA_STATE_CLOSE, camera_->getCameraState());

    retval = camera_->close();
    EXPECT_EQ(CAMERA_ERROR_DEVICE_CLOSE, retval);
    EXPECT_EQ(CAMERA_STATE_CLOSE, camera_->getCameraState());

    retval = camera_->deinit();
}

TEST_F(SDKTest, StartPreview)
{
    int retval = camera_->init(subsystem);
    retval = camera_->open(devname);

    stream_format_t fmt ;
    fmt.stream_height = 480;
    fmt.stream_width = 640;
    fmt.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;

    retval = camera_->startPreview(fmt,IOMODE_DMABUF);
    EXPECT_EQ(CAMERA_ERROR_NONE, retval);
    EXPECT_EQ(CAMERA_STATE_PREVIEW, camera_->getCameraState());

    retval = camera_->stopPreview();
    retval = camera_->close();
    retval = camera_->deinit();
}

TEST_F(SDKTest, StopPreview)
{
    int retval = camera_->init(subsystem);
    retval = camera_->open(devname);

    stream_format_t fmt ;
    fmt.stream_height = 480;
    fmt.stream_width = 640;
    fmt.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
    retval = camera_->startPreview(fmt,IOMODE_DMABUF);

    retval = camera_->stopPreview();
    EXPECT_EQ(CAMERA_ERROR_NONE, retval);
    EXPECT_EQ(CAMERA_STATE_OPEN, camera_->getCameraState());

    retval = camera_->close();
    retval = camera_->deinit();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
