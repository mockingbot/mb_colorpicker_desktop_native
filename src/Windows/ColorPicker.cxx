#include "ColorPicker.hxx"

#include "../Instance.hxx"
#include "../Predefined.hxx"


void
PreRun_Mode_Normal()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}


void
PostRun_Mode_Normal()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}


void
PreRun(class Instance* instance)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    auto inst_info = instance->InstanceInfo();
    auto exec_mode = inst_info->CommandLineParameter<int>(L"--mode=");

    switch(exec_mode)
    {
        case 0:
            PreRun_Mode_Normal();
        break;
        default:
            // pass
        break;
    }
}


void
PostRun(class Instance* instance)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    auto inst_info = instance->InstanceInfo();
    auto exec_mode = inst_info->CommandLineParameter<int>(L"--mode=");

    switch(exec_mode)
    {
        case 0:
            PostRun_Mode_Normal();
        break;
        default:
            // pass
        break;
    }
}

