# ###########################################################
# this part is for Model->GretlDataset references tracking
# ###########################################################
import atexit

# global list to track model instances
_gretl_models = []

def register_model(model):
    """Register a model instance for cleanup before datasets."""
    _gretl_models.append(model)

def cleanup_models():
    """Ensures all models are deleted before datasets at interpreter exit."""
    global _gretl_models
    for model in _gretl_models:
        model.__del__() # forces model destruction
    _gretl_models.clear()

# Register cleanup function to run when Python exits
#atexit.register(cleanup_models)
# ###########################################################